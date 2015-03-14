// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the queue from:
//
// C.M. Kirsch, M. Lippautz, and H. Payer. Fast and Scalable k-FIFO Queues.
// Technical Report 2012-04, Department of Computer Sciences, University of
// Salzburg, June 2012.

#ifndef SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_
#define SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/queue.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"
#include "util/random.h"

#define BSKFIFO_WASTE 1

#ifdef BSKFIFO_WASTE
#define PTR_ALIGNMENT (4096)
#define ITEM_PAD (128 *  4)
#else
#define PTR_ALIGNMENT (128)
#define ITEM_PAD (128)
#endif  // BSKFIFO_WASTE

namespace scal {

template<typename T>
class BoundedSizeKFifo : public Queue<T> {
 public:
  BoundedSizeKFifo(uint64_t k, uint64_t num_segments);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef TaggedValue<uint64_t> SegmentPtr;
  typedef AtomicTaggedValue<uint64_t, PTR_ALIGNMENT, 128> AtomicSegmentPtr;
  typedef TaggedValue<T> Item;
  typedef AtomicTaggedValue<T, 0, ITEM_PAD> AtomicItem;

  _always_inline bool find_index(
      uint64_t start_index, bool empty, int64_t *item_index, Item* old);
  _always_inline bool advance_head(const SegmentPtr& head_old);
  _always_inline bool advance_tail(const SegmentPtr& tail_old);
  _always_inline bool queue_full(
      const SegmentPtr& head_old, const SegmentPtr& tail_old);
  _always_inline bool segment_not_empty(const SegmentPtr& head_old);
  _always_inline bool not_in_valid_region(uint64_t tail_old_pointer,
                                  uint64_t tail_current_pointer,
                                  uint64_t head_current_pointer);
  _always_inline bool in_valid_region(uint64_t tail_old_pointer,
                              uint64_t tail_current_pointer,
                              uint64_t head_current_pointer);
  _always_inline bool committed(
      const SegmentPtr& tail_old, const Item& new_item, uint64_t item_index);

  uint64_t queue_size_;
  size_t k_;
  AtomicSegmentPtr* head_;
  AtomicSegmentPtr* tail_;
  AtomicItem* queue_;
  uint8_t pad_[
    128
        - sizeof(queue_size_)
        - sizeof(k_)
        - sizeof(head_)
        - sizeof(tail_)
        - sizeof(queue_)];
};


template<typename T>
BoundedSizeKFifo<T>::BoundedSizeKFifo(uint64_t k, uint64_t num_segments)
    : queue_size_(k * num_segments)
    , k_(k)
    , head_(new AtomicSegmentPtr())
    , tail_(new AtomicSegmentPtr())
    , queue_(static_cast<AtomicItem*>(
          CallocAligned(k * num_segments, sizeof(AtomicItem), 64))) {
}


template<typename T>
bool BoundedSizeKFifo<T>::find_index(
    uint64_t start_index, bool empty, int64_t *item_index, Item* old) {
  const uint64_t random_index = pseudorand() % k_;
  uint64_t index;
  for (size_t i = 0; i < k_; i++) {
    index = (start_index + ((random_index + i) % k_)) % queue_size_;
    *old = queue_[index].load();
    if ((empty && old->value() == (T)NULL)
        || (!empty && old->value() != (T)NULL)) {
      *item_index = index;
      return true;
    }
  }
  return false;
}


template<typename T>
bool BoundedSizeKFifo<T>::advance_head(const SegmentPtr& head_old) {
  return head_->swap(
      head_old, SegmentPtr((head_old.value() + k_) % queue_size_, head_old.tag() + 1));
}


template<typename T>
bool BoundedSizeKFifo<T>::advance_tail(const SegmentPtr& tail_old) {
  return tail_->swap(
      tail_old, SegmentPtr((tail_old.value() + k_) % queue_size_, tail_old.tag() + 1));
}


template<typename T>
bool BoundedSizeKFifo<T>::queue_full(
    const SegmentPtr& head_old, const SegmentPtr& tail_old) {
  if (((tail_old.value() + k_) % queue_size_) == head_old.value() &&
      (head_old.value() == head_->load().value())) {
    return true;
  }
  return false;
}


template<typename T>
bool BoundedSizeKFifo<T>::segment_not_empty(const SegmentPtr& head_old) {
  const uint64_t start = head_old.value();
  for (size_t i = 0; i < k_; i++) {
    if (queue_[(start + i) % queue_size_].load().value() != (T)NULL) {
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
bool BoundedSizeKFifo<T>::committed(const SegmentPtr& tail_old,
                                    //uint64_t tail_old_pointer,
                                    //AtomicValue<T> *new_item,
                                    const Item& new_item,
                                    uint64_t item_index) {
  if (queue_[item_index].load() != new_item)  {
    return true;
  }

  SegmentPtr tail_current = tail_->load();
  SegmentPtr head_current = head_->load();
  if (in_valid_region(tail_old.value(), tail_current.value(),
                      head_current.value())) {
    return true;
  } else if (not_in_valid_region(tail_old.value(), tail_current.value(),
                                 head_current.value())) {
    if (!queue_[item_index].swap(
          new_item, Item((T)NULL, new_item.tag() + 1))) {
      return true;
    }
  } else {
    if (head_->swap(
          head_current, SegmentPtr(head_current.value(),
                                   head_current.tag() + 1))) {
      return true;
    }
    if (!queue_[item_index].swap(
          new_item, Item((T)NULL, new_item.tag() + 1))) { 
      return true;
    }
  }
  return false;
}


template<typename T>
bool BoundedSizeKFifo<T>::dequeue(T *item) {
  SegmentPtr tail_old;
  SegmentPtr head_old;
  int64_t item_index;
  Item old_item;
  bool found_idx;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    found_idx = find_index(head_old.value(), false, &item_index, &old_item);
    if (head_old == head_->load()) {
      if (found_idx) {
        if (head_old.value() == tail_old.value()) {
          advance_tail(tail_old);
        }
        if (queue_[item_index].swap(
              old_item, Item((T)NULL, old_item.tag() + 1))) {
          *item = old_item.value();
          return true;
        }
      } else {
        if ((head_old.value() == tail_old.value()) &&
            (tail_old.value() == tail_->load().value())) {
          return false;
        }
        advance_head(head_old);
      }
    }
  }
}


template<typename T>
bool BoundedSizeKFifo<T>::enqueue(T item) {
  TaggedValue<T>::CheckCompatibility(item);
  if (item == (T)NULL) {
    printf("%s: unable to enqueue NULL or equivalent value\n", __func__);
    abort();
  }
  SegmentPtr tail_old;
  SegmentPtr head_old;
  int64_t item_index;
  Item old_item;
  bool found_idx;
  while (true) {
    tail_old = tail_->load();
    head_old = head_->load();
    found_idx = find_index(tail_old.value(), true, &item_index, &old_item);
    if (tail_old == tail_->load()) {
      if (found_idx) {
        const Item new_item(item, old_item.tag() + 1);
        if (queue_[item_index].swap(old_item, new_item)) {
          if (committed(tail_old, new_item, item_index)) {
            return true;
          }
        }
      } else {
        if (queue_full(head_old, tail_old)) {
          if (segment_not_empty(head_old) &&
              (head_old.value() == head_->load().value())) {
            return false;
          }
          advance_head(head_old);
        }
        advance_tail(tail_old);
      }
    }
  }
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_BOUNDEDSIZE_KFIFO_H_
