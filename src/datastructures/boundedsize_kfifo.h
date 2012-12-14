// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Implementing the queue from:
//
// C.M. Kirsch, M. Lippautz, and H. Payer. Fast and Scalable k-FIFO Queues.
// Technical Report 2012-04, Department of Computer Sciences, University of
// Salzburg, June 2012.

#ifndef SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_
#define SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>  // Used for placement new.

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

template<typename T>
class BoundedSizeKFifo {
 public:
  static BoundedSizeKFifo<T> *get_aligned(
      uint64_t k, uint64_t num_segments, size_t alignment) {
    using scal::malloc_aligned;
    void *mem = malloc_aligned(sizeof(BoundedSizeKFifo<T>), alignment);
    BoundedSizeKFifo<T>* kfifo = new(mem) BoundedSizeKFifo<T>(k, num_segments);
    return kfifo;
  }

  BoundedSizeKFifo(uint64_t k, uint64_t num_segments);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  static const uint64_t kPtrAlignment = scal::kCachePrefetch;
  static const int64_t kNoIndexFound = -1;

  uint64_t queue_size_;
  size_t k_;
  AtomicValue<T> **queue_;
  AtomicValue<uint64_t> *head_;
  AtomicValue<uint64_t> *tail_;

  void find_index(uint64_t start_index, bool empty, int64_t *item_index,
                  AtomicValue<T> *old);
  bool advance_head(AtomicValue<uint64_t> head_old);
  bool advance_tail(AtomicValue<uint64_t> tail_old);
  bool segment_not_empty(uint64_t head_old_pointer);
  bool queue_full(uint64_t head_old_pointer, uint64_t tail_old_pointer);
  bool committed(uint64_t tail_old_pointer, AtomicValue<T> *new_item,
                 uint64_t item_index);
  bool not_in_valid_region(uint64_t tail_old_pointer,
                           uint64_t tail_current_pointer,
                           uint64_t head_current_pointer);
  bool in_valid_region(uint64_t tail_old_pointer,
                       uint64_t tail_current_pointer,
                       uint64_t head_current_pointer);
};

template<typename T>
BoundedSizeKFifo<T>::BoundedSizeKFifo(uint64_t k, uint64_t num_segments) {
  using scal::malloc_aligned;
  k_ = k;
  queue_size_ = k * num_segments;
  queue_ = static_cast<AtomicValue<T>**>(calloc(
      queue_size_, sizeof(AtomicValue<T>*)));
  for (uint64_t i = 0; i < queue_size_; i++) {
    queue_[i] = scal::get<AtomicValue<T>>(kPtrAlignment * 4);
  }

  // Allocate kPtrAligned head and tail ``pointers''.
  head_ = scal::get_aligned<AtomicValue<uint64_t>>(kPtrAlignment);
  tail_ = scal::get_aligned<AtomicValue<uint64_t>>(kPtrAlignment);
}

template<typename T>
void BoundedSizeKFifo<T>::find_index(uint64_t start_index,
                                     bool empty,
                                     int64_t *item_index,
                                     AtomicValue<T> *old) {
  uint64_t random_index = pseudorand() % k_;
  uint64_t index;
  *item_index = kNoIndexFound;
  for (size_t i = 0; i < k_; i++) {
    index = (start_index + ((random_index + i) % k_)) % queue_size_;
    *old = *queue_[index];
    if ((empty && old->value() == (T)NULL)
        || (!empty && old->value() != (T)NULL)) {
      *item_index = index;
      return;
    }
  }
}

template<typename T>
bool BoundedSizeKFifo<T>::advance_head(AtomicValue<uint64_t> head_old) {
    AtomicValue<uint64_t> newcp(head_old.value() + k_,
                                      head_old.aba() + 1);
    return head_->cas(head_old, newcp);
}

template<typename T>
bool BoundedSizeKFifo<T>::advance_tail(AtomicValue<uint64_t> tail_old) {
    AtomicValue<uint64_t> newcp(tail_old.value() + k_,
                                      tail_old.aba() + 1);
    return tail_->cas(tail_old, newcp);
}

template<typename T>
bool BoundedSizeKFifo<T>::queue_full(uint64_t head_old_pointer,
                                     uint64_t tail_old_pointer) {
  AtomicValue<uint64_t> head_current = *head_;
  if ((head_old_pointer == head_current.value())
      && (((tail_old_pointer + k_) % queue_size_) == head_old_pointer)) {
    return true;
  }
  return false;
}

template<typename T>
bool BoundedSizeKFifo<T>::segment_not_empty(uint64_t head_old_pointer) {
  for (size_t i = 0; i < k_; i++) {
    if (queue_[(head_old_pointer + i) % queue_size_]->value() != (T)NULL) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool BoundedSizeKFifo<T>::in_valid_region(uint64_t tail_old_pointer,
                                          uint64_t tail_current_pointer,
                                          uint64_t head_current_pointer) {
  bool wrap_around = (tail_current_pointer < head_current_pointer)
                     ? true : false;
  if (!wrap_around) {
    return (head_current_pointer < tail_old_pointer
            && tail_old_pointer <= tail_current_pointer) ? true : false;
  }
  return (head_current_pointer < tail_old_pointer
          || tail_old_pointer <= tail_current_pointer) ? true : false;
}

template<typename T>
bool BoundedSizeKFifo<T>::not_in_valid_region(uint64_t tail_old_pointer,
                                              uint64_t tail_current_pointer,
                                              uint64_t head_current_pointer) {
  bool wrap_around = (tail_current_pointer < head_current_pointer)
                     ? true : false;
  if (!wrap_around) {
    return (tail_old_pointer < tail_current_pointer
            || head_current_pointer < tail_old_pointer) ? true : false;
  }
  return (tail_old_pointer < tail_current_pointer
          && head_current_pointer < tail_old_pointer) ? true : false;
}

template<typename T>
bool BoundedSizeKFifo<T>::committed(uint64_t tail_old_pointer,
                                    AtomicValue<T> *new_item,
                                    uint64_t item_index) {
  if (queue_[item_index]->value() != new_item->value()) {
    return true;
  }
  AtomicValue<uint64_t> tail_current = *tail_;
  AtomicValue<uint64_t> head_current = *head_;

  if (in_valid_region(tail_old_pointer, tail_current.value(),
                      head_current.value())) {
    return true;
  } else if (not_in_valid_region(tail_old_pointer, tail_current.value(),
                                 head_current.value())) {
    AtomicValue<T> newcp((T)NULL, new_item->aba() + 1);
    if (!queue_[item_index]->cas(*new_item, newcp)) {
      return true;
    }
  } else {
    AtomicValue<uint64_t> newcp = head_current;
    newcp.set_aba(newcp.aba() + 1);
    if (head_->cas(head_current, newcp)) {
      return true;
    }
    AtomicValue<T> newcp2((T)NULL, new_item->aba() + 1);
    if (!queue_[item_index]->cas(*new_item, newcp2)) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool BoundedSizeKFifo<T>::dequeue(T *item) {
  AtomicValue<uint64_t> tail_old;
  AtomicValue<uint64_t> head_old;
  int64_t item_index;
  AtomicValue<T> old_item;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    find_index(head_old.value(), false, &item_index, &old_item);
    if (head_old.raw() == head_->raw()) {
      if (item_index != kNoIndexFound) {
        if (head_old.value() == tail_old.value()) {
          advance_tail(tail_old);
        }
        AtomicValue<T> newcp((T)NULL, old_item.aba() + 1);
        if (queue_[item_index]->cas(old_item, newcp)) {
          *item = old_item.value();
          return true;
        }
      } else {
        if (head_old.value() == tail_old.value()
            && tail_old.value() == tail_->value()) {
          return false;
        }
        advance_head(head_old);
      }
    }
  }
}

template<typename T>
bool BoundedSizeKFifo<T>::enqueue(T item) {
  if (item == (T)NULL) {
    printf("%s: unable to enqueue NULL or equivalent value\n", __func__);
    abort();
  }
  AtomicValue<uint64_t> tail_old;
  AtomicValue<uint64_t> head_old;
  int64_t item_index;
  AtomicValue<T> old_item;
  while (true) {
    tail_old = *tail_;
    head_old = *head_;
    find_index(tail_old.value(), true, &item_index, &old_item);
    if (tail_old.raw() == tail_->raw()) {
      if (item_index != kNoIndexFound) {
        AtomicValue<T> newcp(item, old_item.aba() + 1);
        if (queue_[item_index]->cas(old_item, newcp)) {
          if (committed(tail_old.value(), &newcp, item_index)) {
            return true;
          }
        }
      } else {
        if (queue_full(head_old.value(), tail_old.value())) {
          if (segment_not_empty(head_old.value())) {
            return false;
          }
          advance_head(head_old);
        }
        advance_tail(tail_old);
      }
    }
  }
}

#endif  // SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_
