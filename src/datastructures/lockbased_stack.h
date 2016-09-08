// Copyright (c) 2015 the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_LOCKBASED_STACK_
#define SCAL_DATASTRUCTURES_LOCKBASED_STACK_

#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>     // strerror_r

#include "datastructures/stack.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"

namespace scal {

namespace detail {

template<typename S>
class Node : public ThreadLocalMemory<64>  {
 public:
  explicit Node(S item) : value(item) {}
  Node<S>* next;
  S value;
};

}  // namespace detail

template<typename T>
class LockBasedStack : public Stack<T> {
 public:
  LockBasedStack();
  bool push(T item);
  bool pop(T *item);

  inline bool put(T item) { return push(item); }
  inline State put_state() { return top_->load().tag(); }
  inline bool empty() { return top_->load().value() == NULL; }
  inline bool get_return_put_state(T *item, State* put_state);

 private:
  typedef detail::Node<T> Node;
  typedef TaggedValue<Node*> NodePtr;
  typedef AtomicTaggedValue<Node*, 64, 64> AtomicNodePtr;

  AtomicNodePtr* top_;

  pthread_mutex_t *global_lock_;

  inline void check_error(const char *std, int rc) {
    if (rc != 0) {
      char err[256];
      char *tmp = strerror_r(rc, err, 256);
      fprintf(stderr, "error: %s: %s %s\n", std, err, tmp);
      abort();
    }
  }
};

template<typename T>
LockBasedStack<T>::LockBasedStack() : top_(new AtomicNodePtr()) {
  global_lock_ = static_cast<pthread_mutex_t*>(
                  MallocAligned(sizeof(pthread_mutex_t), 64));
  int rc = pthread_mutex_init(global_lock_, NULL);
  check_error("pthread_mutex_init", rc);
}

template<typename T>
bool LockBasedStack<T>::push(T item) {
  Node *n = new Node(item);
  NodePtr top_old;

  int rc = pthread_mutex_lock(global_lock_);
  check_error("pthread_mutex_lock", rc);

  top_old = top_->load();
  n->next = top_old.value();
  top_->store(NodePtr(n, top_old.tag() + 1));

  rc = pthread_mutex_unlock(global_lock_);
  check_error("pthread_mutex_unlock", rc);
  return true;
}

template<typename T>
bool LockBasedStack<T>::pop(T *item) {
  NodePtr top_old;

  int rc = pthread_mutex_lock(global_lock_);
  check_error("pthread_mutex_lock", rc);

  top_old = top_->load();
  if (top_old.value() == NULL) {
    rc = pthread_mutex_unlock(global_lock_);
    check_error("pthread_mutex_unlock", rc);
    return false;
  }
  top_->store(NodePtr(top_old.value()->next, top_old.tag() + 1));
  *item = top_old.value()->value;

  rc = pthread_mutex_unlock(global_lock_);
  check_error("pthread_mutex_unlock", rc);
  return true;
}

template<typename T>
bool LockBasedStack<T>::get_return_put_state(T* item, State* put_state) {
  NodePtr top_old;
  NodePtr top_new;

  int rc = pthread_mutex_lock(global_lock_);
  check_error("pthread_mutex_lock", rc);

  top_old = top_->load();
  if (top_old.value() == NULL) {
    *put_state = top_old.tag();
    rc = pthread_mutex_unlock(global_lock_);
    check_error("pthread_mutex_unlock", rc);
    return false;
  }
  top_->store(NodePtr(top_old.value()->next, top_old.tag() + 1));
  *item = top_old.value()->value;
  *put_state = top_old.tag();

  rc = pthread_mutex_unlock(global_lock_);
  check_error("pthread_mutex_unlock", rc);
  return true;
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_LOCKBASED_STACK_
