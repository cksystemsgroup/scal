// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the k-stack from:
//
// T. A. Henzinger, C. M. Kirsch, H. Payer, and A. Sokolova. Quantitative
// relaxation of concurrent data structures. In Proceedings of the 40th annual
// ACM SIGPLAN-SIGACT symposium on Principles of programming languages, POPL
// ’13, New York, NY, USA, 2013. ACM.

#ifndef SCAL_DATASTRUCTURES_KSTACK_H_
#define SCAL_DATASTRUCTURES_KSTACK_H_

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <limits>

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"
#include "util/threadlocals.h"

namespace kstack_details {

template<typename T>
struct KSegment {
  static uint64_t K;

  uint64_t *remove;
  AtomicPointer<KSegment*> *next;
  AtomicValue<T> **items;

  KSegment() {
    this->remove = scal::tlget<uint64_t>(128);
    *(this->remove) = 0;
    this->next = scal::tlget<AtomicPointer<KSegment*>>(128);
    this->items = static_cast<AtomicValue<T>**>(scal::tlcalloc_aligned(
        K, sizeof(*items), 128));
    for (uint64_t i = 0; i < K; i++) {
      this->items[i] = scal::tlget<AtomicValue<T>>(128*4);
    }
  }
};

template<typename T>
uint64_t KSegment<T>::K = 1;

}  // namespace kstack_details

template<typename T>
class KStack {
 public:
  KStack(uint64_t k, uint64_t num_threads);
  bool push(T item);
  bool pop(T *item);

 private:
  typedef kstack_details::KSegment<T> KSegment;

  static const uint64_t kNoIndexFound = std::numeric_limits<uint64_t>::max();
  static const uint64_t kSegmentSize = scal::kPageSize;
  static const uint64_t kPtrAlignment = scal::kCachePrefetch;


  inline bool is_empty(KSegment* segment);
  inline void find_index(KSegment *segment,
                         bool empty,
                         uint64_t *item_index,
                         AtomicValue<T> *old);
  void try_add_new_ksegment(AtomicPointer<KSegment*> top_old);
  void try_remove_ksegment(AtomicPointer<KSegment*> top_old);
  bool committed(AtomicPointer<KSegment*> top_old,
                 AtomicValue<T> item_new,
                 uint64_t index);

  AtomicPointer<KSegment*> *top_;
  AtomicRaw **item_records_;
  uint64_t k_;
  uint64_t num_threads_;
};

template<typename T>
KStack<T>::KStack(uint64_t k, uint64_t num_threads) {
  k_ = k;
  num_threads_ = num_threads;
  KSegment::K = k_;
  top_ = scal::tlget<AtomicPointer<KSegment*>>(kSegmentSize);
  top_->weak_set_value(scal::tlget<KSegment>(kSegmentSize));
  item_records_ = static_cast<AtomicRaw**>(scal::calloc_aligned(
      num_threads_, sizeof(*item_records_), kPtrAlignment));
  for (uint64_t i = 0; i < num_threads_; i++) {
    item_records_[i] = static_cast<AtomicRaw*>(scal::tlcalloc_aligned(
        k_, sizeof(*(item_records_[i])), kPtrAlignment));
  }
}

template<typename T>
bool KStack<T>::is_empty(KSegment* segment) {
  // Distributed Queue style empty check.
  uint64_t thread_id = threadlocals_get()->thread_id;
  uint64_t random_index = pseudorand() % k_;
  uint64_t index;
  AtomicValue<T> item_old;
  for (uint64_t i =0; i < k_; i++) {
    index = (random_index + i) % k_;
    item_old = *(segment->items[index]);
    AtomicValue<T> item_empty((T)NULL, item_old.aba() + 1);
    if (item_old.value() != (T)NULL) {
      return false;
    } else {
     item_records_[thread_id][index] = item_old.raw(); 
    }
  }
  for (uint64_t i = 0; i < k_; i++) {
    index = (random_index + i) % k_;
    item_old = *(segment->items[index]);
    AtomicValue<T> item_empty((T)NULL, item_old.aba() + 1);
    if (item_old.raw() != item_records_[thread_id][index]) {
      return false;
    }
  }
  return true;
}

template<typename T>
void KStack<T>::try_add_new_ksegment(AtomicPointer<KSegment*> top_old) {
  if (top_->raw() == top_old.raw()) {
    KSegment *segment_new = scal::tlget<KSegment>(kSegmentSize);
    segment_new->next->weak_set_value(top_old.value());
    AtomicPointer<KSegment*> top_new(segment_new, top_old.aba());
    top_->cas(top_old, top_new);
  }
}

template<typename T>
void KStack<T>::try_remove_ksegment(AtomicPointer<KSegment*> top_old) {
  AtomicPointer<KSegment*> next = *(top_->value()->next);
  if (top_->raw() == top_old.raw()) {
    if (next.value() != NULL) {
      __sync_fetch_and_add((top_old.value()->remove), 1);
      if (is_empty(top_old.value())) {
        AtomicPointer<KSegment*> top_new(top_old.value()->next->value(),
                                         top_old.aba() + 1);
        if (top_->cas(top_old, top_new)) {
          return;
        }
      }
      __sync_fetch_and_sub((top_old.value()->remove), 1);
    }
  }
}

template<typename T>
bool KStack<T>::committed(AtomicPointer<KSegment*> top_old,
                          AtomicValue<T> item_new,
                          uint64_t index) {
  if (top_old.value()->items[index]->raw() != item_new.raw()) {
    return true;
  } else if (*(top_old.value()->remove) == 0) {
    return true;
  } else if (*(top_old.value()->remove) >= 1) {
    AtomicValue<T> item_empty((T)NULL, item_new.aba() + 1);
    if (top_->raw() != top_old.raw()) {
      if (!top_old.value()->items[index]->cas(item_new, item_empty)) {
        return true;
      }
    } else {
      AtomicPointer<KSegment*> top_new(top_old.value(), top_old.aba() + 1);
      if (top_->cas(top_old, top_new)) {
        return true;
      }
      if (!top_old.value()->items[index]->cas(item_new, item_empty)) {
        return true;
      }
    }
  }
  return false;
}

template<typename T>
void KStack<T>::find_index(KSegment *segment,
                           bool empty,
                           uint64_t *item_index,
                           AtomicValue<T> *old) {
  uint64_t random_index = pseudorand() % k_;
  uint64_t i;
  *item_index = kNoIndexFound;
  for (uint64_t _cnt = 0; _cnt < k_; _cnt++) {
    i = (random_index + _cnt) % k_;
    *old = *(segment->items[i]);
    if ((empty && old->value() == (T)NULL) ||
        (!empty && old->value() != (T)NULL)) {
      *item_index = i;
      return;
    }
  }
}

template<typename T>
bool KStack<T>::push(T item) {
  AtomicPointer<KSegment*> top_old;
  AtomicValue<T> item_old;
  uint64_t item_index;
  while (true) {
    top_old = *top_;
    find_index(top_old.value(), true, &item_index, &item_old);
    if (top_->raw() == top_old.raw()) {
      if (item_index != kNoIndexFound) {
        AtomicValue<T> item_new(item, item_old.aba() + 1);
        if (top_old.value()->items[item_index]->cas(item_old, item_new)) {
          if (committed(top_old, item_new, item_index)) {
            return true;
          }
        }
      } else {
        try_add_new_ksegment(top_old);
      }
    }
  }
}

template<typename T>
bool KStack<T>::pop(T *item) {
  AtomicPointer<KSegment*> top_old;
  AtomicValue<T> item_old;
  uint64_t item_index;
  while (true) {
    top_old = *top_;
    find_index(top_old.value(), false, &item_index, &item_old);
    if (top_->raw() == top_old.raw()) {
      if (item_index != kNoIndexFound) {
        AtomicValue<T> item_empty((T)NULL, item_old.aba() + 1);
        if (top_->value()->items[item_index]->cas(item_old, item_empty)) {
          *item = item_old.value();
          return true;
        }
      } else {
        if (top_old.value()->next->value() == NULL) {  // is last segment
          if (is_empty(top_old.value())) {
            if (top_->raw() == top_old.raw()) {
              return false;
            }
          }
        } else {
          try_remove_ksegment(top_old);
        }
      }
    }
  }
}

#endif  // SCAL_DATASTRUCTURES_KSTACK_H_
