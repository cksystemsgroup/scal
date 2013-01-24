// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the non-blocking lock-free stack from:
//
// R. K. Treiber. Systems Programming: Coping with Parallelism. RJ 5118, IBM
// Almaden Research Center, April 1986.

#ifndef SCAL_DATASTRUCTURES_TREIBER_STACK_H_
#define SCAL_DATASTRUCTURES_TREIBER_STACK_H_

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"

namespace ts_internal {

template<typename T>
struct Node {
  AtomicPointer<Node*> next;
  T data;
};

}  // namespace ts_internal

template<typename T>
class TreiberStack {
 public:
  TreiberStack();
  bool push(T item);
  bool pop(T *item);

 private:
  typedef ts_internal::Node<T> Node;

  AtomicPointer<Node*> *top_;
};

template<typename T>
TreiberStack<T>::TreiberStack() {
  top_ = scal::get<AtomicPointer<Node*> >(scal::kCachePrefetch);
}

template<typename T>
bool TreiberStack<T>::push(T item) {
  Node *n = scal::tlget<Node>(scal::kCachePrefetch);
  n->data = item;
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  do {
    top_old = *top_;
    n->next.weak_set_value(top_old.value());
    top_new.weak_set_value(n);
    top_new.weak_set_aba(top_old.aba() + 1);
  } while (!top_->cas(top_old, top_new));
  return true;
}

template<typename T>
bool TreiberStack<T>::pop(T *item) {
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  do {
    top_old = *top_;
    if (top_old.value() == NULL) {
      return false;
    }
    top_new.weak_set_value(top_old.value()->next.value());
    top_new.weak_set_aba(top_old.aba() + 1);
  } while (!top_->cas(top_old, top_new));
  *item = top_old.value()->data;
  return true;
}

#endif  // SCAL_DATASTRUCTURES_TREIBER_STACK_H_
