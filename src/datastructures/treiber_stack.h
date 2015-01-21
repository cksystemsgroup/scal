// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the non-blocking lock-free stack from:
//
// R. K. Treiber. Systems Programming: Coping with Parallelism. RJ 5118, IBM
// Almaden Research Center, April 1986.

#ifndef SCAL_DATASTRUCTURES_TREIBER_STACK_H_
#define SCAL_DATASTRUCTURES_TREIBER_STACK_H_

#include "datastructures/distributed_queue_interface.h"
#include "datastructures/stack.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"

namespace scal {

namespace detail {

template<typename T>
struct Node : ThreadLocalMemory<64> {
  explicit Node(T item) : next(NULL), data(item) { }

  Node<T>* next;
  T data;
};

}  // namespace detail


template<typename T>
class TreiberStack : public Stack<T> {
 public:
  TreiberStack();
  bool push(T item);
  bool pop(T *item);

  // Satisfy the DistributedQueueInterface

  inline bool put(T item) {
    return push(item);
  }

  inline AtomicRaw empty_state() {
    return top_->load().tag();
  }

  inline bool empty() {
    return top_->load().value() == NULL;
  }

  inline bool get_return_empty_state(T *item, AtomicRaw *state);

 private:
  typedef detail::Node<T> Node;
  typedef TaggedValue<Node*> NodePtr;
  typedef AtomicTaggedValue<Node*, 64, 64> AtomicNodePtr;

  AtomicNodePtr* top_;
};


template<typename T>
TreiberStack<T>::TreiberStack() : top_(new AtomicNodePtr()) {
}


template<typename T>
bool TreiberStack<T>::push(T item) {
  Node* n = new Node(item);
  NodePtr top_old;
  NodePtr top_new;
  do {
    top_old = top_->load();
    n->next = top_old.value();
    top_new = NodePtr(n, top_old.tag() + 1);
  } while (!top_->swap(top_old, top_new));
  return true;
}


template<typename T>
bool TreiberStack<T>::pop(T *item) {
  NodePtr top_old;
  NodePtr top_new;
  do {
    top_old = top_->load();
    if (top_old.value() == NULL) {
      return false;
    }
    top_new = NodePtr(top_old.value()->next, top_old.tag() + 1);
  } while (!top_->swap(top_old, top_new));
  *item = top_old.value()->data;
  return true;
}


template<typename T>
bool TreiberStack<T>::get_return_empty_state(T* item, AtomicRaw* state) {
  NodePtr top_old;
  NodePtr top_new;
  do {
    top_old = top_->load();
    if (top_old.value() == NULL) {
      *state = top_old.tag();
      return false;
    }
    top_new = NodePtr(top_old.value()->next, top_old.tag() + 1);
  } while (!top_->swap(top_old, top_new));
  *item = top_old.value()->data;
  *state = top_old.tag();
  return true;
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_TREIBER_STACK_H_
