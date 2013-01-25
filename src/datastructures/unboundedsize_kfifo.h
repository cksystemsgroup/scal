// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the queue from:
//
// C.M. Kirsch, M. Lippautz, and H. Payer. Fast and Scalable k-FIFO Queues.
// Technical Report 2012-04, Department of Computer Sciences, University of
// Salzburg, June 2012.

#ifndef SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_
#define SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/queue.h"
#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

namespace uskfifo_details {

template<typename T>
struct KSegment {
  AtomicPointer<KSegment*> next;
  uint64_t k;
  bool deleted;
  AtomicValue<T> **items;
};

}  // namespace uskfifo_details

template<typename T>
class UnboundedSizeKFifo : public Queue<T> {
 public:
  explicit UnboundedSizeKFifo(uint64_t k);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef uskfifo_details::KSegment<T> KSegment;

  static const uint64_t kPtrAlignment = scal::kCachePrefetch;
  static const int64_t kNoIndexFound = -1;

  AtomicPointer<KSegment*> *head_;
  AtomicPointer<KSegment*> *tail_;
  uint64_t k_;

  inline KSegment* ksegment_new(void);
  void advance_head(AtomicPointer<KSegment*> head_old);
  void advance_tail(AtomicPointer<KSegment*> tail_old);
  void find_index(KSegment *start_index, bool empty,
                  int64_t *item_index, AtomicValue<T> *old);
  bool committed(AtomicPointer<KSegment*> tail_old,
                 AtomicValue<T> *new_item, uint64_t item_index);

  inline AtomicPointer<KSegment*> get_head(void) {
    return *head_;
  }

  inline AtomicPointer<KSegment*> get_tail(void) {
    return *tail_;
  }
};

template<typename T>
uskfifo_details::KSegment<T>* UnboundedSizeKFifo<T>::ksegment_new() {
  KSegment *ksegment = static_cast<KSegment*>(scal::tlcalloc(
      1, sizeof(KSegment)));
  ksegment->k = k_;
  ksegment->items = static_cast<AtomicValue<T>**>(scal::tlcalloc(
      ksegment->k, sizeof(AtomicValue<T>*)));
  for (uint64_t i = 0; i < ksegment->k; i++) {
    // Segments are contiguous without any alignment.
    ksegment->items[i] = scal::tlget<AtomicValue<T> >(0);
  }
  ksegment->deleted = false;
  return ksegment;
}

template<typename T>
UnboundedSizeKFifo<T>::UnboundedSizeKFifo(uint64_t k) {
  k_ = k;
  KSegment *ksegment = ksegment_new();

  head_ = scal::get<AtomicPointer<KSegment*> >(scal::kPageSize);
  tail_ = scal::get<AtomicPointer<KSegment*> >(scal::kPageSize);
  head_->weak_set_value(ksegment);
  tail_->weak_set_value(ksegment);
}

template<typename T>
void UnboundedSizeKFifo<T>::advance_head(
    AtomicPointer<uskfifo_details::KSegment<T>*> head_old) {
  AtomicPointer<KSegment*> head_current = get_head();
  if (head_current.raw() == head_old.raw()) {
    AtomicPointer<KSegment*> tail_current = get_tail();
    AtomicPointer<KSegment*> tail_next_ksegment = tail_current.value()->next;
    AtomicPointer<KSegment*> head_next_ksegment = head_current.value()->next;
    if (head_current.raw() == get_head().raw()) {
      if (head_current.value() == tail_current.value()) {
        if (tail_next_ksegment.value() == NULL) {
          return;
        }
        if (tail_current.raw() == get_tail().raw()) {
          tail_next_ksegment.set_aba(tail_current.aba() + 1);
          tail_->cas(tail_current, tail_next_ksegment);
        }
      }
      head_old.value()->deleted = true;
      head_next_ksegment.set_aba(head_old.aba() + 1);
      head_->cas(head_old, head_next_ksegment);
    }
  }
}

template<typename T>
void UnboundedSizeKFifo<T>::advance_tail(
    AtomicPointer<uskfifo_details::KSegment<T>*> tail_old) {
  AtomicPointer<KSegment*> tail_current = get_tail();
  AtomicPointer<KSegment*> next_ksegment;
  if (tail_current.raw() == tail_old.raw()) {
    next_ksegment = tail_old.value()->next;
    if (tail_old.raw() == get_tail().raw()) {
      if (next_ksegment.value() != NULL) {
        next_ksegment.set_aba(next_ksegment.aba() + 1);
        tail_->cas(tail_old, next_ksegment);
      } else {
        KSegment *ksegment = ksegment_new();
        AtomicPointer<KSegment*> new_ksegment(
            ksegment, next_ksegment.aba() + 1);
        if (tail_old.value()->next.cas(next_ksegment, new_ksegment)) {
          new_ksegment.set_aba(tail_old.aba() + 1);
          tail_->cas(tail_old, new_ksegment);
        }
      }
    }
  }
}

template<typename T>
void UnboundedSizeKFifo<T>::find_index(
    uskfifo_details::KSegment<T> *start_index, bool empty, int64_t *item_index,
    AtomicValue<T> *old) {
  uint64_t random_index = pseudorand() % start_index->k;
  uint64_t index;
  *item_index = kNoIndexFound;
  for (size_t i = 0; i < start_index->k; i++) {
    index = ((random_index + i) % start_index->k);
    *old = *start_index->items[index];
    if ((empty && old->value() == (T)NULL)
        || (!empty && old->value() != (T)NULL)) {
      *item_index = index;
      return;
    }
  }
}

template<typename T>
bool UnboundedSizeKFifo<T>::committed(
    AtomicPointer<uskfifo_details::KSegment<T>*> tail_old,
    AtomicValue<T> *new_item,
    uint64_t item_index) {
  if (tail_old.value()->items[item_index]->raw() != new_item->raw()) {
    return true;
  }
  AtomicPointer<KSegment*> head_current = get_head();
  AtomicValue<T> empty_item((T)NULL, 0);

  if (tail_old.value()->deleted == true) {
    // Not in queue anymore.
    if (!tail_old.value()->items[item_index]->cas(*new_item, empty_item)) {
      return true;
    }
  } else if (tail_old.value() == head_current.value()) {
    AtomicPointer<KSegment*> head_new = head_current;
    head_new.weak_set_aba(head_new.aba() + 1);
    if (head_->cas(head_current, head_new)) {
      return true;
    }
    if (!tail_old.value()->items[item_index]->cas(*new_item, empty_item)) {
      return true;
    }
  } else if (tail_old.value()->deleted == false) {
    // In queue and inserted tail not head.
    return true;
  } else {
    if (!tail_old.value()->items[item_index]->cas(*new_item, empty_item)) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool UnboundedSizeKFifo<T>::dequeue(T *item) {
  AtomicPointer<KSegment*> tail_old;
  AtomicPointer<KSegment*> head_old;
  int64_t item_index;
  AtomicValue<T> old_item;
  while (true) {
    head_old = get_head();
    find_index(head_old.value(), false, &item_index, &old_item);
    tail_old = get_tail();
    if (head_old.raw() == head_->raw()) {
      if (item_index != kNoIndexFound) {
        if (head_old.value() == tail_old.value()) {
          advance_tail(tail_old);
        }
        AtomicValue<T> newcp((T)NULL, old_item.aba() + 1);
        if (head_old.value()->items[item_index]->cas(old_item, newcp)) {
          *item = old_item.value();
          return true;
        }
      } else {
        if (head_old.value() == tail_old.value()) {
          return false;
        }
        advance_head(head_old);
      }
    }
  }
}

template<typename T>
bool UnboundedSizeKFifo<T>::enqueue(T item) {
  if (item == (T)NULL) {
    printf("%s: unable to enqueue NULL or equivalent value\n", __func__);
    abort();
  }
  AtomicPointer<KSegment*> tail_old;
  AtomicPointer<KSegment*> head_old;
  int64_t item_index;
  AtomicValue<T> old_item;
  while (true) {
    tail_old = get_tail();
    head_old = get_head();
    find_index(tail_old.value(), true, &item_index, &old_item);
    if (tail_old.raw() == tail_->raw()) {
      if (item_index != kNoIndexFound) {
        AtomicValue<T> newcp(item, old_item.aba() + 1);
        if (tail_old.value()->items[item_index]->cas(old_item, newcp)) {
          if (committed(tail_old, &newcp, item_index)) {
            return true;
          }
        }
      } else {
        advance_tail(tail_old);
      }
    }
  }
}

#endif  // SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_
