// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
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
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/operation_logger.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {

template<typename S>
class Node : public ThreadLocalMemory<kCachePrefetch>  {
 public:
  explicit Node(S item) : value(item) {}

  AtomicTaggedValue<Node<S>*> next;
  S value;
};

}  // namespace detail


template<typename T>
class MSQueue : public Queue<T> {
 public:
  typedef detail::Node<T> Node;

  MSQueue();
  bool enqueue(T item);
  bool dequeue(T *item);

  inline const TaggedValue<Node*> head() const {
    return head_->load();
  }

  inline const TaggedValue<Node*> tail() const {
    return tail_->load();
  }

 private:
  typedef TaggedValue<Node*> NodePtr;
  typedef AtomicTaggedValue<Node*, scal::kCachePrefetch> AtomicNodePtr;

  AtomicNodePtr* head_;
  AtomicNodePtr* tail_;
};


template<typename T>
MSQueue<T>::MSQueue()
    : head_(new AtomicNodePtr()),
      tail_(new AtomicNodePtr()) {
  Node* node = new Node(static_cast<T>(NULL));
  head_->store(NodePtr(node, 0));
  tail_->store(NodePtr(node, 0));
}


template<typename T>
bool MSQueue<T>::enqueue(T item) {
  Node* node = new Node(item);
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    tail_old = tail_->load();
    next = tail_old.value()->next.load();
    if (tail_old == tail_->load()) {
      if (next.value() == NULL) {
        if (tail_old.value()->next.swap(
                next, NodePtr(node, next.tag() + 1))) {
          tail_->swap(tail_old, NodePtr(node, tail_old.tag() + 1));
          break;
        }
      } else {
        tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
      }
    }
  }
  return true;
}


template<typename T>
bool MSQueue<T>::dequeue(T *item) {
  NodePtr head_old;
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    next = head_old.value()->next.load();
    if (head_->load() == head_old) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          return false;
        }
        tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
      } else {
        *item = next.value()->value;
        if (head_->swap(
                head_old, NodePtr(next.value(), head_old.tag() + 1))) {
          break;
        }
      }
    }
  }
  return true;
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_MS_QUEUE_H_
