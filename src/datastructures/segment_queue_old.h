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

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/random.h"

namespace sq_details {

template<typename T>
struct Pair {
  T value;
  bool deleted;
};

template<typename T>
struct Node {
  Pair<T> **segment;
  AtomicPointer<Node*> *next;
};

}  // namespace sq_details

template<typename T>
class SegmentQueue {
 public:
  explicit SegmentQueue(uint64_t s);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef sq_details::Node<T> Node;
  typedef sq_details::Pair<T> Pair;

  uint64_t s_;
  AtomicPointer<Node*> *head_;
  AtomicPointer<Node*> *tail_;

  sq_details::Node<T>* node_new(uint64_t s);
  AtomicPointer<Node*> get_tail(void);
  AtomicPointer<Node*> get_head(void);
  void tail_segment_create(AtomicPointer<Node*> my_tail);
  void head_segment_remove(AtomicPointer<Node*> my_head);
};

template<typename T>
SegmentQueue<T>::SegmentQueue(uint64_t s) {
  s_ = s;
  Node *n = node_new(s);
  tail_ = AtomicPointer<Node*>::get_aligned(128);
  head_ = AtomicPointer<Node*>::get_aligned(128);
  tail_->weak_set_value(n);
  head_->weak_set_value(n);
}

template<typename T>
sq_details::Node<T>* SegmentQueue<T>::node_new(uint64_t s) {
  Node *n = scal::tlget_aligned<Node>(128);
  n->next = AtomicPointer<Node*>::get_aligned(128);
  n->segment = static_cast<Pair**>(scal::tlcalloc(s, sizeof(Pair*)));
  for (uint64_t i = 0; i < s; i++) {
    n->segment[i] = static_cast<Pair*>(scal::tlcalloc(1, sizeof(Pair)));
    n->segment[i]->deleted = false;
    n->segment[i]->value = (T)NULL;
  }
  return n;
}

template<typename T>
AtomicPointer<sq_details::Node<T>*> SegmentQueue<T>::get_tail(void) {
  AtomicPointer<Node*> next;
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> tail_old;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = *(tail_old.value()->next);
    if (tail_old.raw() == tail_->raw()
        && head_old.raw() == head_->raw()) {
      if (next.value() == NULL) {
        if (head_old.value() == tail_old.value()) {
          AtomicPointer<Node*> tmp(NULL, tail_old.aba());
          return tmp;  // only sentinel left
        }
        return tail_old;
      }
      AtomicPointer<Node*> tmp(next.value(), tail_old.aba());
      tail_->cas(tail_old, tmp);
    }
  }
}

template<typename T>
AtomicPointer<sq_details::Node<T>*> SegmentQueue<T>::get_head(void) {
  AtomicPointer<Node*> old_head;
  AtomicPointer<Node*> old_tail;
  AtomicPointer<Node*> next;
  while (true) {
    old_head = *head_;
    old_tail = *tail_;
    next = *(old_head.value()->next);
    if (old_head.raw() == head_->raw()) {
      if (old_head.value() == old_tail.value()) {
        if (next.value() == NULL) {
          AtomicPointer<Node*> tmp(NULL, 0);
          return tmp;
        }
      }
      return next;
    }
  }
}

template<typename T>
void SegmentQueue<T>::tail_segment_create(
    AtomicPointer<sq_details::Node<T>*> my_tail) {
  AtomicPointer<Node*> old_tail;
  AtomicPointer<Node*> next;
  Node *n = NULL;
  while (true) {
    old_tail = *tail_;
    next = *(old_tail.value()->next);
    if (old_tail.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        if ((my_tail.value() == NULL && my_tail.aba() == old_tail.aba())
            || old_tail.raw() == my_tail.raw()) {
          n = node_new(s_);
          AtomicPointer<Node*> tmp(n, 0);
          if (old_tail.value()->next->cas(next, tmp)) {
            break;
          }
        }
        return;  // somebody else made it
      } else {
        AtomicPointer<Node*> tmp(next.value(), old_tail.aba() + 1);
        tail_->cas(old_tail, tmp);
        return;
      }
    }
  }
  AtomicPointer<Node*> tmp(n, old_tail.aba() + 1);
  tail_->cas(old_tail, tmp);
}

template<typename T>
void SegmentQueue<T>::head_segment_remove(
    AtomicPointer<sq_details::Node<T>*> my_head) {
  AtomicPointer<Node*> old_head;
  AtomicPointer<Node*> old_tail;
  AtomicPointer<Node*> next;
  while (true) {
    old_head = *head_;
    old_tail = *tail_;
    next = *(old_head.value()->next);
    if (old_head.value() == head_->value()) {
      if (old_head.value() == old_tail.value()) {
        if (next.value() == NULL) {
          return;  // we don't removethe sentinel
        }
        AtomicPointer<Node*> tmp(next.value(), old_tail.aba() + 1);
        tail_->cas(old_tail, tmp);
      } else {
        if (next.raw() == my_head.raw()) {
          // still the same, let's push the sentinel by one
          AtomicPointer<Node*> tmp(next.value(), old_head.aba() + 1);
          head_->cas(old_head, tmp);
        }
        return;  // someone else made it
      }
    }
  }
}

template<typename T>
bool SegmentQueue<T>::enqueue(T item) {
  AtomicPointer<Node*> tail = get_tail();
  while (tail.value() == NULL) {  // only sentinel node in the queue
    tail_segment_create(tail);
    tail = get_tail();
  }
  uint64_t item_index;
  uint64_t rand;
  while (true) {
    rand = pseudorand() % s_;
    for (uint64_t i = 0; i <= s_; i++) {
      item_index = (i + rand) % s_;
      if (tail.value()->segment[item_index]->value != (T)NULL) {  // optimization
        continue;
      }
      if (__sync_bool_compare_and_swap(
          &(tail.value()->segment[item_index]->value), NULL, item)) {
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
bool SegmentQueue<T>::dequeue(T *item) {
  AtomicPointer<Node*> head;
  uint64_t rand;
  uint64_t item_index;
  bool found_null = false;
  while (true) {
    head = get_head();
    if (head.value() == NULL) {
      return false;
    }
    rand = pseudorand() % s_;
    for (uint64_t i = 0; i < s_; i++) {
      item_index = (i + rand) % s_;
      if (head.value()->segment[item_index]->value == (T)NULL) {
        found_null = true;
        continue;
      }
      if (__sync_bool_compare_and_swap(
          &(head.value()->segment[item_index]->deleted),
          false, true)) {
        *item = head.value()->segment[item_index]->value;
        return true;
      }
    }
    if (found_null) {
      return false;
    }
    head_segment_remove(head);
  }
}

#endif  // SCAL_DATASTRUCTURES_SEGMENT_QUEUE_H_
