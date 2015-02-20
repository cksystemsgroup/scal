// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the k-relaxed queue from:
//
// C.M. Kirsch, M. Lippautz, and H. Payer. Fast and scalable, lock-free k-fifo
// queues. In Proc. International Conference on Parallel Computing Technologies
// (PaCT), LNCS, pages 208-223. Springer, October 2013.

#ifndef SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_
#define SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/queue.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"
#include "util/random.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {

template<typename T>
class KSegment : public ThreadLocalMemory<64> {
 public:
  typedef TaggedValue<T> Item;
  typedef AtomicTaggedValue<T, 0, 128> AtomicItem;
  typedef TaggedValue<KSegment*> SegmentPtr;
  typedef AtomicTaggedValue<KSegment*, 0, 128> AtomicSegmentPtr;

  explicit KSegment(uint64_t k)
      : deleted_(0)
      , k_(k)
      , next_(SegmentPtr(NULL, 0))
      , items_(static_cast<AtomicItem*>(
            ThreadLocalAllocator::Get().CallocAligned(
                k, sizeof(AtomicItem), 64))) {
  }

  inline uint64_t k() { return k_; }
  inline uint8_t deleted() { return deleted_; }
  inline void set_deleted() { deleted_ = 1; }

  inline SegmentPtr next() { return next_.load(); }
  inline bool atomic_set_next(
      const SegmentPtr& old_next, const SegmentPtr& new_next) {
    return next_.swap(old_next, new_next);
  }

  inline Item item(uint64_t index) {
    return items_[index].load();
  }
  inline bool atomic_set_item(
      uint64_t index, const Item& old_item, const Item& new_item) {
    return items_[index].swap(old_item, new_item);
  }

 private:
  uint8_t deleted_;
  uint8_t pad_[63];
  uint64_t k_;
  AtomicSegmentPtr next_;
  AtomicItem* items_;
};

}  // namespace detail


template<typename T>
class UnboundedSizeKFifo : public Queue<T> {
 public:
  explicit UnboundedSizeKFifo(uint64_t k);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef detail::KSegment<T> KSegment;
  typedef typename KSegment::SegmentPtr SegmentPtr;
  typedef typename KSegment::Item Item;
  typedef typename KSegment::AtomicItem AtomicItem;
  typedef AtomicTaggedValue<KSegment*, 4096, 4096> AtomicSegmentPtr;

  void advance_head(const SegmentPtr& head_old);
  void advance_tail(const SegmentPtr& tail_old);
  bool find_index(KSegment* const start_index, bool empty,
                  int64_t *item_index, Item* old);
  bool committed(const SegmentPtr& tail_old,
                 const Item& new_item, uint64_t item_index);

#ifdef LOCALLY_LINEARIZABLE
  struct Marker {
    SegmentPtr value;
    uint8_t padding[64 - sizeof(SegmentPtr)];
  };

  inline void SetLastSegment(SegmentPtr tail) {
    markers[ThreadContext::get().thread_id()].value = tail;
  }

  inline bool IsLastSegment(SegmentPtr tail) {
    return markers[ThreadContext::get().thread_id()].value == tail;
  }

  Marker markers[kMaxThreads];
#endif  // LOCALLY_LINEARIZABLE
  AtomicSegmentPtr* head_;
  AtomicSegmentPtr* tail_;
  uint64_t k_;
};


template<typename T>
UnboundedSizeKFifo<T>::UnboundedSizeKFifo(uint64_t k)
    : k_(k) {
  const SegmentPtr new_segment(new KSegment(k_), 0);
  head_ = new AtomicSegmentPtr(new_segment);
  tail_ = new AtomicSegmentPtr(new_segment);
}


template<typename T>
void UnboundedSizeKFifo<T>::advance_head(const SegmentPtr& head_old) {
  const SegmentPtr head_current = head_->load();
  if (head_current == head_old) {
    const SegmentPtr tail_current = tail_->load();
    const SegmentPtr tail_next_ksegment = tail_current.value()->next();
    const SegmentPtr head_next_ksegment = head_current.value()->next();
    if (head_current == head_->load()) {
      if (head_current.value() == tail_current.value()) {
        if (tail_next_ksegment.value() == NULL) {
          return;
        }
        if (tail_current == tail_->load()) {
          tail_->swap(tail_current, SegmentPtr(tail_next_ksegment.value(),
                                               tail_current.tag() + 1));
        }
      }
      head_old.value()->set_deleted();
      head_->swap(
          head_old, SegmentPtr(head_next_ksegment.value(), head_old.tag() + 1));
    }
  }
}


template<typename T>
void UnboundedSizeKFifo<T>::advance_tail(const SegmentPtr& tail_old) {
  const SegmentPtr tail_current = tail_->load();
  SegmentPtr next_ksegment;
  if (tail_current == tail_old) {
    next_ksegment = tail_old.value()->next();
    if (tail_old == tail_->load()) {
      if (next_ksegment.value() != NULL) {
        tail_->swap(tail_old, SegmentPtr(next_ksegment.value(),
                                         next_ksegment.tag() + 1));
      } else {
        KSegment* segment = new KSegment(k_);
        const SegmentPtr new_ksegment(segment, next_ksegment.tag() + 1);
        if (tail_old.value()->atomic_set_next(next_ksegment, new_ksegment)) {
          tail_->swap(
              tail_old, SegmentPtr(new_ksegment.value(), tail_old.tag() + 1));
        } else {
          delete segment;
        }
      }
    }
  }
}


template<typename T>
bool UnboundedSizeKFifo<T>::find_index(
    detail::KSegment<T>* const start_index, bool empty, int64_t *item_index,
    Item* old) {
  const uint64_t k = start_index->k();
  const uint64_t random_index = hwrand() % k;
  uint64_t index;
  for (size_t i = 0; i < k; i++) {
    index = ((random_index + i) % k);
    *old = start_index->item(index);
    if ((empty && old->value() == (T)NULL)
        || (!empty && old->value() != (T)NULL)) {
      *item_index = index;
      return true;
    }
  }
  return false;
}


template<typename T>
bool UnboundedSizeKFifo<T>::committed(
    const SegmentPtr& tail_old, const Item& new_item, uint64_t item_index) {
  if (tail_old.value()->item(item_index) != new_item) {
    return true;
  }
  const SegmentPtr head_current = head_->load();
  const Item empty_item((T)NULL, 0);

  if (tail_old.value()->deleted() == true) {
    // Insert tail segment has been removed.
    if (!tail_old.value()->atomic_set_item(item_index, new_item, empty_item)) {
      // We are fine if element still has been removed.
      return true;
    }
  } else if (tail_old.value() == head_current.value()) {
    // Insert tail segment is now head.
    if (head_->swap(head_current,
                    SegmentPtr(head_current.value(), head_current.tag() + 1))) {
      // We are fine if we can update head and thus fail any concurrent
      // advance_head attempts.
      return true;
    }
    if (!tail_old.value()->atomic_set_item(item_index, new_item, empty_item)) {
      // We are fine if element still has been removed.
      return true;
    }
  } else if (tail_old.value()->deleted() == false) {
    // Insert tail segment still not deleted.
    return true;
  } else {
    // Head and tail moved beyond this segment. Try to remove the item.
    if (!tail_old.value()->atomic_set_item(item_index, new_item, empty_item)) {
      // We are fine if element still has been removed.
      return true;
    }
  }
  return false;
}


template<typename T>
bool UnboundedSizeKFifo<T>::dequeue(T *item) {
  SegmentPtr tail_old;
  SegmentPtr head_old;
  int64_t item_index = 0;
  Item old_item;
  bool found_idx;
  while (true) {
    head_old = head_->load();
    found_idx = find_index(head_old.value(), false, &item_index, &old_item);
    tail_old = tail_->load();
    if (head_old == head_->load()) {
      if (found_idx) {
        if (head_old.value() == tail_old.value()) {
          advance_tail(tail_old);
        }
        const Item newcp((T)NULL, old_item.tag() + 1);
        if (head_old.value()->atomic_set_item(item_index, old_item, newcp)) {
          *item = old_item.value();
          return true;
        }
      } else {
        if ((head_old.value() == tail_old.value()) &&
            (tail_old == tail_->load())) {
          return false;
        }
        advance_head(head_old);
      }
    }
  }
}


template<typename T>
bool UnboundedSizeKFifo<T>::enqueue(T item) {
  TaggedValue<T>::CheckCompatibility(item);
  if (item == (T)NULL) {
    printf("%s: unable to enqueue NULL or equivalent value\n", __func__);
    abort();
  }
  SegmentPtr tail_old;
  SegmentPtr head_old;
  int64_t item_index = 0;
  Item old_item;
  bool found_idx;
  while (true) {
    tail_old = tail_->load();
#ifdef LOCALLY_LINEARIZABLE
    if (IsLastSegment(tail_old)) {
      advance_tail(tail_old);
      continue;
    }
#endif  // LOCALLY_LINEARIZABLE
    head_old = head_->load();
    found_idx = find_index(tail_old.value(), true, &item_index, &old_item);
    if (tail_old == tail_->load()) {
      if (found_idx) {
        const Item newcp(item, old_item.tag() + 1);
        if (tail_old.value()->atomic_set_item(item_index, old_item, newcp)) {
          if (committed(tail_old, newcp, item_index)) {
#ifdef LOCALLY_LINEARIZABLE
            SetLastSegment(tail_old);
#endif  // LOCALLY_LINEARIZABLE
            return true;
          }
        }
      } else {
        advance_tail(tail_old);
      }
    }
  }
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_UNBOUNDEDSIZE_KFIFO_H_
