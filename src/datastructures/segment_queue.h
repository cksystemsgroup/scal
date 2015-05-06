// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Implementing the segment queue from:
//
// Y. Afek, G. Korland, and E. Yanovsky. Quasi-linearizability: Relaxed
// consistency for improved concurrency. In Proc. Conference on Principles of
// Distributed Systems (OPODIS), pages 395–410. Springer, 2010.

#ifndef SCAL_DATASTRUCTURES_SEGMENT_QUEUE_H_
#define SCAL_DATASTRUCTURES_SEGMENT_QUEUE_H_

#include <inttypes.h>

#include "datastructures/queue.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/random.h"

namespace scal {

namespace detail {

template<typename T>
class Pair : public ThreadLocalMemory<64> {
 public:
  _always_inline explicit Pair(T value)
      : deleted(false) {
    this->value = value;
  }
 
  T value;
  bool deleted;
};


template<typename T>
class Node  : public ThreadLocalMemory<64> {
 public:
  typedef TaggedValue<Node*> NodePtr;
  typedef AtomicTaggedValue<Node*, 0, 64> AtomicNodePtr;

  _always_inline Node(uint64_t s) 
      : segment(static_cast<Pair<T>**>(
          ThreadLocalAllocator::Get().CallocAligned(
              s, sizeof(Pair<T>*), 64)))
      , next_(NodePtr(NULL, 0)) {
    for (uint64_t i = 0; i < s; i++) {
      segment[i] = new Pair<T>((T)NULL);
    }
  }

  _always_inline NodePtr next() { return next_.load(); }
  _always_inline bool atomic_set_next(
      const NodePtr& old_next, const NodePtr& new_next) { 
    return next_.swap(old_next, new_next);
  }

  _always_inline T item(uint64_t index) {
    return segment[index]->value;
  }

  _always_inline bool atomic_set_item(
      uint64_t index, T old_item, T new_item) {
    return __sync_bool_compare_and_swap(
        &segment[index]->value, old_item, new_item);
  }

  _always_inline bool mark_deleted(uint64_t index) {
    return __sync_bool_compare_and_swap(
        &segment[index]->deleted, false, true);
  }

 private:
  Pair<T>** segment;
  AtomicNodePtr next_;
};

}  // namespace detail


template<typename T>
class SegmentQueue : public Queue<T> {
 public:
  explicit SegmentQueue(uint64_t s);
  bool enqueue(T item);
  bool dequeue(T* item);

 private:
  typedef detail::Node<T> Node;
  typedef typename detail::Node<T>::NodePtr NodePtr;
  typedef AtomicTaggedValue<Node*, 4*128, 4*128> AtomicNodePtr;

  NodePtr get_tail();
  NodePtr get_head();
  void tail_segment_create(const typename detail::Node<T>::NodePtr& my_tail);
  void head_segment_remove(const typename detail::Node<T>::NodePtr& my_head);

  AtomicNodePtr* head_;
  AtomicNodePtr* tail_;
  uint64_t s_;
};


template<typename T>
SegmentQueue<T>::SegmentQueue(uint64_t s) 
    : s_(s) {
  const NodePtr new_node(new Node(s), 0);
  head_ = new AtomicNodePtr(new_node);
  tail_ = new AtomicNodePtr(new_node);
}


template<typename T>
bool SegmentQueue<T>::enqueue(T item) {
  NodePtr tail = get_tail();
  while (tail.value() == NULL) {
    tail_segment_create(tail);
    tail = get_tail();
  }
  uint64_t item_index;
  uint64_t rand;
  while (true) {
    rand = hwrand() % s_;
    for (uint64_t i = 0; i <= s_; i++) {
      item_index = (i + rand) % s_;
      if (tail.value()->item(item_index) != (T)NULL) {  // optimization
        continue;
      }
      if (tail.value()->atomic_set_item(item_index, (T)NULL, item)) {
        return true;
      }
    }
    do {
      tail_segment_create(tail);
      tail = get_tail();
    } while (tail.value() == NULL);
  }
}


template<typename T>
bool SegmentQueue<T>::dequeue(T* item) {
  NodePtr head;
  uint64_t rand;
  uint64_t item_index;
  bool found_null = false;
  while (true) {
    head = get_head();
    if (head.value() == NULL) {
      return false;
    }
    rand = hwrand() % s_;
    for (uint64_t i = 0; i < s_; i++) {
      item_index = (i + rand) % s_;
      if (head.value()->item(item_index) == (T)NULL) {
        found_null = true;
        continue;
      }
      if (head.value()->mark_deleted(item_index)) {
        *item = head.value()->item(item_index);
        return true;
      }
    }
    if (found_null) { 
      return false;
    }
    head_segment_remove(head);
  }
}


template<typename T>
typename detail::Node<T>::NodePtr SegmentQueue<T>::get_tail() {
  NodePtr next;
  NodePtr head_old;
  NodePtr tail_old;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    next = tail_old.value()->next();
    if ((tail_old == tail_->load()) &&
        (head_old == head_->load())) {
      if (next.value() == NULL) {
        if (head_old == tail_old) {
          return NodePtr(NULL, tail_old.tag());  // only sentinel left
        }
        return tail_old;
      }
      NodePtr tmp(next.value(), tail_old.tag());
      tail_->swap(tail_old, tmp);
    }
  }
}


template<typename T>
typename detail::Node<T>::NodePtr SegmentQueue<T>::get_head() {
  NodePtr next;
  NodePtr head_old;
  NodePtr tail_old;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    next = head_old.value()->next();
    if (head_old == head_->load()) {
      if (head_old == tail_old) {
        if (next.value() == NULL) {
          return NodePtr(NULL, 0);
        }
      }
      return next;
    }
  }
}


template<typename T>
void SegmentQueue<T>::tail_segment_create(
    const typename detail::Node<T>::NodePtr& my_tail) {
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    tail_old = tail_->load();
    next = tail_old.value()->next();
    if (tail_old == tail_->load()) {
      if (next.value() == NULL) {
        if ((my_tail.value() == NULL && my_tail.tag() == tail_old.tag()) ||
            tail_old == my_tail) {
          const NodePtr new_next(new Node(s_), 0);
          if (tail_old.value()->atomic_set_next(next, new_next)) {
            break;
          }
        }
        return;  // somebody else made it
      } else {
        const NodePtr tmp(next.value(), tail_old.tag() + 1);
        tail_->swap(tail_old, tmp);
        return;
      }
    }
  }
}


template<typename T>
void SegmentQueue<T>::head_segment_remove(
    const typename detail::Node<T>::NodePtr& my_head) {
  NodePtr head_old;
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    next = head_old.value()->next();
    if (head_old == head_->load()) {
      if (head_old == tail_old) {
        if (next.value() == NULL) {
          return;  // We don't remove sentinel node.
        }
        NodePtr new_tail(next.value(), tail_old.tag() + 1);
        tail_->swap(tail_old, new_tail);
      } else {
        if (next == my_head) {
          // Still the same, let's push the sentinel by one.
          NodePtr new_head(next.value(), head_old.tag() + 1);
          head_->swap(head_old, new_head);
        }
        return;  // someone else made it
      }
    }
  }
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_SEGMENT_QUEUE_H_
