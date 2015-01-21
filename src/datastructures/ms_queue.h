// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the queue from:
//
// M. Michael and M. Scott. Simple, fast, and practical non-blocking and
// blocking concurrent queue algorithms. In Proc. Symposium on Principles of
// Distributed Computing (PODC), pages 267–275. ACM, 1996.

// ... and lots of variants

#ifndef SCAL_DATASTRUCTURES_MS_QUEUE_H_
#define SCAL_DATASTRUCTURES_MS_QUEUE_H_

#include <new>

#include "datastructures/distributed_queue_interface.h"
#include "datastructures/queue.h"
#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/operation_logger.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace ms_details {

template<typename S>
struct Node {
  AtomicPointer<Node<S>*> next;
  S value;
};

}  // namespace ms_details

template<typename T>
class MSQueue : public Queue<T>, public DistributedQueueInterface<T> {
 public:
  /*
   * Correct size would need to check consistency of state and fix tail ptr.
   * We need a fast approximate version here.
   */
  inline uint64_t approx_size(void) const {
    AtomicPointer<Node*> tail_old = *tail_;
    AtomicPointer<Node*> head_old = *head_;
    return tail_old.aba() - head_old.aba();
  }

  inline bool empty() {
    return head_->value() == tail_->value();
  }

  inline AtomicPointer<ms_details::Node<T>*> get_head(void) const {
    return *head_;
  }

  inline AtomicPointer<ms_details::Node<T>*> get_tail(void) const {
    return *tail_;
  }

  inline AtomicRaw get_tail_raw(void) const {
    return tail_->raw();
  }

  MSQueue(void);
  bool enqueue(T item);
  bool dequeue(T *item);

  bool dequeue_return_tail(T *item, AtomicRaw *tail_raw);
  bool try_enqueue(T item, AtomicPointer<ms_details::Node<T>*> tail_old);
  uint8_t try_dequeue(T *item,
                      AtomicPointer<ms_details::Node<T>*> head_old,
                      uint64_t *tail_raw);

  // Satisfy the DistributedQueueInterface

  inline bool put(T item) {
    return enqueue(item);
  }

  inline AtomicRaw empty_state() {
    return tail_->raw();
  }

  inline bool get_return_empty_state(T *item, AtomicRaw *state) {
    return dequeue_return_tail(item, state);
  }

 private:
  typedef ms_details::Node<T> Node;

  AtomicPointer<Node*> *head_;
  AtomicPointer<Node*> *tail_;

  inline Node* node_new(T item) const {
    Node *node = scal::tlget_aligned<Node>(scal::kCachePrefetch);
    node->next.weak_set_value(NULL);
    node->next.weak_set_aba(0);
    node->value = item;
    return node;
  }
};

template<typename T>
MSQueue<T>::MSQueue(void) {
  head_ = scal::get_aligned<AtomicPointer<Node*> >(4 * 128);
  tail_ = scal::get_aligned<AtomicPointer<Node*> >(4 * 128);
  Node *node = node_new((T)NULL);
  head_->weak_set_value(node);
  tail_->weak_set_value(node);
}

template<typename T>
bool MSQueue<T>::enqueue(T item) {
  Node *node = node_new(item);
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> next;
  while (true) {
    tail_old = *tail_;
    next = tail_old.value()->next;
    if (tail_old.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        AtomicPointer<Node*> new_next(node, next.aba() + 1);
        if (tail_old.value()->next.cas(next, new_next)) {
          scal::StdOperationLogger::get().linearization();
          break;
        }
      } else {
        AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
      }
    }
  }
  AtomicPointer<Node*> tail_new(node, tail_old.aba() + 1);
  tail_->cas(tail_old, tail_new);
  return true;
}

template<typename T>
bool MSQueue<T>::dequeue(T *item) {
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> next;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = head_old.value()->next;
    if (head_->raw() == head_old.raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          scal::StdOperationLogger::get().linearization();
          return false;
        }
        AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
      } else {
        *item = next.value()->value;
        AtomicPointer<Node*> head_new(next.value(), head_old.aba() + 1);
        if (head_->cas(head_old, head_new)) {
          scal::StdOperationLogger::get().linearization();
          break;
        }
      }
    }
  }
  return true;
}

template<typename T>
bool MSQueue<T>::dequeue_return_tail(T *item, AtomicRaw *tail_raw) {
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> next;
  while (true) {
    head_old = *head_;
    tail_old = *tail_;
    next = head_old.value()->next;
    if (head_->raw() == head_old.raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          scal::StdOperationLogger::get().linearization();
          *tail_raw = tail_old.raw();
          return false;
        }
        AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
      } else {
        *item = next.value()->value;
        AtomicPointer<Node*> head_new(next.value(), head_old.aba() + 1);
        if (head_->cas(head_old, head_new)) {
          scal::StdOperationLogger::get().linearization();
          *tail_raw = tail_old.raw();
          break;
        }
      }
    }
  }
  return true;
}

template<typename T>
bool MSQueue<T>::try_enqueue(
    T item, AtomicPointer<ms_details::Node<T>*> tail_old) {
  AtomicPointer<Node*> next = tail_old.value()->next;
  if (tail_->raw() == tail_old.raw()) {
    if (next.value() == NULL) {
      Node *node = node_new(item);
      AtomicPointer<Node*> new_next(node, next.aba() + 1);
      if (tail_old.value()->next.cas(next, new_next)) {
        scal::StdOperationLogger::get().linearization();
        AtomicPointer<Node*> tail_new(node, tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
        return true;
      }
    } else {
      AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
      tail_->cas(tail_old, tail_new);
    }
  }
  return false;
}

template<typename T>
uint8_t MSQueue<T>::try_dequeue(
    T *item, AtomicPointer<ms_details::Node<T>*> head_old, uint64_t *tail_raw) {
  AtomicPointer<Node*> tail_old = *tail_;
  AtomicPointer<Node*> next = head_old.value()->next;
  if (head_->raw() == head_old.raw()) {
    if (head_old.value() == tail_old.value()) {
      if (next.value() == NULL) {
        scal::StdOperationLogger::get().linearization();
        *tail_raw = tail_old.aba();
        return 1;  // empty
      }
      AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
      tail_->cas(tail_old, tail_new);
      *tail_raw = tail_new.aba();
    } else {
      *item = next.value()->value;
      AtomicPointer<Node*> head_new(next.value(), head_old.aba() + 1);
      if (head_->cas(head_old, head_new)) {
        scal::StdOperationLogger::get().linearization();
        *tail_raw = tail_old.aba();
        return 0;  // ok
      }
      *tail_raw = tail_old.aba();
    }
  }
  scal::StdOperationLogger::get().linearization();
  return 2;  // failed
}

#endif  // SCAL_DATASTRUCTURES_MS_QUEUE_H_
