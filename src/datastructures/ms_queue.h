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

#include "datastructures/distributed_data_structure_interface.h"
#include "datastructures/queue.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/operation_logger.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {

template<typename S>
class Node : public ThreadLocalMemory<64>  {
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
  typedef TaggedValue<Node*> NodePtr;

  MSQueue();
  bool enqueue(T item);
  bool dequeue(T *item);

  inline const TaggedValue<Node*> head() const {
    return head_->load();
  }

  inline const TaggedValue<Node*> tail() const {
    return tail_->load();
  }

  inline State put_state() {
    return tail_->load().tag();
  }

  bool get_return_put_state(T* item, State* put_state);

  bool empty();

  bool try_enqueue(T item, uint64_t tal_old_tag);
  uint8_t try_dequeue(T* item, uint64_t head_old_tag, State* put_state);

 private:
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
bool MSQueue<T>::try_enqueue(T item, uint64_t tail_old_tag) {
  NodePtr next;
  NodePtr tail_old;
  tail_old = tail_->load();
  next = tail_old.value()->next.load();
  if (tail_old_tag == tail_old.tag()) {
    if (next.value() == NULL) {
      Node* node = new Node(item);
      if (tail_old.value()->next.swap(
              next, NodePtr(node, next.tag() + 1))) {
        tail_->swap(tail_old, NodePtr(node, tail_old.tag() + 1));
        return true;
      }
    } else {
      tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
    }
  }
  return false;
}


template<typename T>
bool MSQueue<T>::empty() {
  NodePtr head_old;
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    head_old = head_->load();
    tail_old = tail_->load();
    next = head_old.value()->next.load();
    if (head_->load() == head_old) {
      if ((head_old.value() == tail_old.value()) &&
          (next.value() == NULL)) {
        return true;
      }
      return false;
    }
  }
}


template<typename T>
bool MSQueue<T>::dequeue(T* item) {
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


template<typename T>
uint8_t MSQueue<T>::try_dequeue(
    T* item, uint64_t head_old_tag, State* put_state) {
  NodePtr head_old = head_->load();
  NodePtr tail_old = tail_->load();
  NodePtr next = head_old.value()->next.load();
  if (head_old_tag == head_old.tag()) {
    if (head_old.value() == tail_old.value()) {
      if (next.value() == NULL) {
        *put_state = tail_old.tag();
        return 1;
      }
      tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
    } else {
      *item = next.value()->value;
      if (head_->swap(
              head_old, NodePtr(next.value(), head_old.tag() + 1))) {
        return 0;
      }
    }
  }
  return 2;
}


template<typename T>
bool MSQueue<T>::get_return_put_state(T *item, State* put_state) {
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
          *put_state = tail_old.tag();
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
  *put_state = head_old.tag();
  return true;
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_MS_QUEUE_H_
