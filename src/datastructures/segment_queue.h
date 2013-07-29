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

#include <assert.h>
#include <inttypes.h>

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/random.h"
#include "util/platform.h"
#include "datastructures/queue.h"

namespace sq_details {

template<typename T>
struct Pair {
  T value;
  bool deleted;
};

template<typename T>
struct Node {
  Pair<T> **segment;
  AtomicPointer<Node<T>*> *next;
};

}  // namespace sq_details

template<typename T>
class SegmentQueue : public Queue<T> {
 public:
  explicit SegmentQueue(uint64_t s);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  uint64_t s_;
  AtomicPointer<sq_details::Node<T>*> *head_;
  AtomicPointer<sq_details::Node<T>*> *tail_;
//  ConcurrentPointer<sq_details::Node<T>*> *head_;
//  ConcurrentPointer<sq_details::Node<T>*> *tail_;

  sq_details::Node<T>* node_new(uint64_t s);

  AtomicPointer<sq_details::Node<T>*> get_tail(void);
  AtomicPointer<sq_details::Node<T>*> get_head(void);

  void tail_segment_create(AtomicPointer<sq_details::Node<T>*> my_tail);
  void head_segment_remove(AtomicPointer<sq_details::Node<T>*> my_head);
};

template<typename T>
SegmentQueue<T>::SegmentQueue(uint64_t s) {
  using sq_details::Node;
  s_ = s;
  Node<T> *n = node_new(s);

  head_ = scal::get_aligned<AtomicPointer<sq_details::Node<T>*> >(4 * 128);
  tail_ = scal::get_aligned<AtomicPointer<sq_details::Node<T>*> >(4 * 128);
  
//  tail_ = ConcurrentPointer<Node<T>*>::get_aligned(128);
//  head_ = ConcurrentPointer<Node<T>*>::get_aligned(128);
  tail_->weak_set_value(n);
  head_->weak_set_value(n);
}

template<typename T>
sq_details::Node<T>* SegmentQueue<T>::node_new(uint64_t s) {
  using scal::tlcalloc;
  using scal::tlcalloc_aligned;
  using sq_details::Pair;
  using sq_details::Node;
  Node<T> *n = static_cast<Node<T>*>(tlcalloc_aligned(1, sizeof(Node<T>), 128));
  n->next = scal::tlget_aligned<AtomicPointer<sq_details::Node<T>*> >(scal::kCachePrefetch);
//  n->next = ConcurrentPointer<Node<T>*>::get_aligned(128);
  n->segment = static_cast<Pair<T>**>(tlcalloc(s, sizeof(Pair<T>*)));
  for (uint64_t i = 0; i < s; i++) {
    n->segment[i] = static_cast<Pair<T>*>(tlcalloc(1, sizeof(Pair<T>)));
    n->segment[i]->deleted = false;
    n->segment[i]->value = (T)NULL;
  }
  assert(n->segment != NULL);
  assert(n->next != NULL);
  return n;
}

template<typename T>
AtomicPointer<sq_details::Node<T>*> SegmentQueue<T>::get_tail(void) {
  using sq_details::Node;
  AtomicPointer<Node<T>*> next;
  AtomicPointer<Node<T>*> head_old;
  AtomicPointer<Node<T>*> tail_old;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = *(tail_old.value()->next);
    if (tail_old.raw() == tail_->raw()
        && head_old.raw() == head_->raw()) {
      if (next.value() == NULL) {
        if (head_old.value() == tail_old.value()) {
          AtomicPointer<Node<T>*> tmp(NULL, tail_old.aba());
          return tmp;  // only sentinel left
        }
        return tail_old;
      }
      AtomicPointer<Node<T>*> tmp(next.value(), tail_old.aba() + 1);
      tail_->cas(tail_old, tmp);
    }
  }
}

template<typename T>
AtomicPointer<sq_details::Node<T>*> SegmentQueue<T>::get_head(void) {
  using sq_details::Node;
  AtomicPointer<Node<T>*> head_old;
  AtomicPointer<Node<T>*> tail_old;
  AtomicPointer<Node<T>*> next;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = *(head_old.value()->next);
    if (head_old.raw() == head_->raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          AtomicPointer<Node<T>*> tmp(NULL, 0);
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
  using sq_details::Node;
  AtomicPointer<Node<T>*> tail_old;
  AtomicPointer<Node<T>*> next;
  Node<T> *n = NULL;
  while (true) {
    tail_old = *tail_;
    next = *(tail_old.value()->next);
    if (tail_old.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        if ((my_tail.value() == NULL && my_tail.aba() == tail_old.aba())
            || tail_old.raw() == my_tail.raw()) {
          n = node_new(s_);
          AtomicPointer<Node<T>*> tmp(n, 0);
          if (tail_old.value()->next->cas(next, tmp)) {
            break;
          }
        }
        return;  // somebody else made it
      } else {
        AtomicPointer<Node<T>*> tmp(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tmp);
        return;
      }
    }
  }
  assert(n != NULL);
  AtomicPointer<Node<T>*> tmp(n, tail_old.aba() + 1);
  tail_->cas(tail_old, tmp);
}

template<typename T>
void SegmentQueue<T>::head_segment_remove(
    AtomicPointer<sq_details::Node<T>*> my_head) {
  using sq_details::Node;
  AtomicPointer<Node<T>*> head_old;
  AtomicPointer<Node<T>*> tail_old;
  AtomicPointer<Node<T>*> next;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = *(head_old.value()->next);
    if (head_old.value() == head_->value()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          return;  // we don't removethe sentinel
        }
        AtomicPointer<Node<T>*> tmp(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tmp);
      } else {
        if (next.raw() == my_head.raw()) {
          // still the same, let's push the sentinel by one
          AtomicPointer<Node<T>*> tmp(next.value(), head_old.aba() + 1);
          head_->cas(head_old, tmp);
        }
        return;  // someone else made it
      }
    }
  }
}

template<typename T>
bool SegmentQueue<T>::enqueue(T item) {
  using sq_details::Node;
  AtomicPointer<Node<T>*> tail = get_tail();
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
  using sq_details::Node;
  AtomicPointer<Node<T>*> head;
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

