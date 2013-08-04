// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the non-blocking lock-free stack from:
//
// R. K. Treiber. Systems Programming: Coping with Parallelism. RJ 5118, IBM
// Almaden Research Center, April 1986.

#ifndef SCAL_DATASTRUCTURES_ELIMINATION_BACKOFF_STACK_H_
#define SCAL_DATASTRUCTURES_ELIMINATION_BACKOFF_STACK_H_

#include <stdint.h>

#include "datastructures/distributed_queue_interface.h"
#include "datastructures/partial_pool_interface.h"
#include "datastructures/stack.h"
#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include <assert.h>
#include "util/threadlocals.h"
#include "util/random.h"

namespace ebs_internal {

  enum Opcode {
  Push = 1,
  Pop  = 2
};

template<typename T>
struct Operation {
  Opcode opcode;
  T data;
};

template<typename T>
struct Node {
  AtomicPointer<Node*> next;
  T data;
};

}  // namespace ebs_internal

template<typename T>
class EliminationBackoffStack : public Stack<T>, public PartialPoolInterface {
 public:
  EliminationBackoffStack(uint64_t num_threads, uint64_t size_collision);
  bool push(T item);
  bool pop(T *item);

  // Satisfy the PartialPoolInterface

  inline uint64_t approx_size(void) const {
    assert(false);
    return 0;
  }

  // Satisfy the DistributedQueueInterface

  inline bool put(T item) {
    return push(item);
  }

  inline AtomicRaw empty_state() {
    return top_->raw();
  }

  inline bool get_return_empty_state(T *item, AtomicRaw *state);

 private:
  typedef ebs_internal::Node<T> Node;
  typedef ebs_internal::Opcode Opcode;
  typedef ebs_internal::Operation<T> Operation;

  AtomicPointer<Node*> *top_;
  volatile Operation* *operations_;

  volatile AtomicValue<uint64_t>* *location_;
  volatile AtomicValue<uint64_t>* *collision_;
  const uint64_t size_collision_;
  bool try_collision(uint64_t thread_id, uint64_t other, T *item);
  bool backoff(Opcode opcode, T *item);
};

template<typename T>
EliminationBackoffStack<T>::EliminationBackoffStack(uint64_t num_threads, uint64_t size_collision) : size_collision_(size_collision) {

  top_ = scal::get<AtomicPointer<Node*> >(scal::kCachePrefetch);

  operations_ = static_cast<volatile Operation**>(calloc(
      num_threads, sizeof(Operation*)));

  location_ = static_cast<volatile AtomicValue<uint64_t>**>(calloc(
        num_threads, sizeof(AtomicValue<uint64_t>*)));

  collision_ = static_cast<volatile AtomicValue<uint64_t>**>(calloc(
        size_collision_, sizeof(AtomicValue<uint64_t>*)));

  for (uint64_t i = 0; i < num_threads; i++) {
    operations_[i] = scal::get<Operation>(scal::kCachePrefetch);
    location_[i] = scal::get<AtomicValue<uint64_t>>(scal::kCachePrefetch);
  }

  for (uint64_t i = 0; i < size_collision_; i++) {
    collision_[i] = scal::get<AtomicValue<uint64_t>>(scal::kCachePrefetch);
  }
}

uint64_t get_position(uint64_t size) {
  return pseudorand() % size;
}

#define EMPTY 0

template<typename T>
bool EliminationBackoffStack<T>::try_collision(uint64_t thread_id, uint64_t other, T *item) {
  
  AtomicValue<T> old_value(other, 0);

  if (operations_[thread_id]->opcode == Opcode::Push) {

    AtomicValue<T> new_value(thread_id, 0);
    if (location_[other]->cas(old_value, new_value)) {
      return true;
    } else {
      return false;
    }
  } else {

    AtomicValue<T> new_value(EMPTY, 0);
    if (location_[other]->cas(old_value, new_value)) {
      *item = operations_[other]->data;
      return true;
    } else {
      return false;
    }
  }
}

template<typename T>
bool EliminationBackoffStack<T>::backoff(Opcode opcode, T *item) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();

  operations_[thread_id]->opcode = opcode;
  operations_[thread_id]->data = *item;

  location_[thread_id]->set_value(thread_id);
  uint64_t position = get_position(size_collision_);
  AtomicValue<uint64_t> him = *collision_[position];

  AtomicValue<T> new_item(thread_id, 0);
  AtomicValue<T> empty_item(0, 0);
  while (!collision_[position]->cas(him, new_item)) {
    him = *collision_[position];
  }

  if (him.value() != EMPTY) {
    AtomicValue<uint64_t> other = *location_[him.value()];
    if (other.value() != 0 && other.value() == him.value() && operations_[other.value()]->opcode != opcode) {
      if (location_[thread_id]->cas(new_item, empty_item)) {
        if (try_collision(thread_id, other.value(), item)) {
         return true;
        } else {
          return false;
        }
      } else {
        if (opcode == Opcode::Pop) {
          *item = operations_[location_[thread_id]->value()]->data;
          location_[thread_id]->set_value(0);
        }
        return true;
      }
    }
  }

  // delay
  //
  
  if (!location_[thread_id]->cas(new_item, empty_item)) {
    if (opcode == Opcode::Pop) {
      *item = operations_[location_[thread_id]->value()]->data;
      location_[thread_id]->set_value(0);
    }
    return true;
  }

  return false;
}
template<typename T>
bool EliminationBackoffStack<T>::push(T item) {
  Node *n = scal::tlget<Node>(0);
  n->data = item;
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  top_new.weak_set_value(n);
  while (true) {
    top_old = *top_;
    n->next.weak_set_value(top_old.value());
    top_new.weak_set_aba(top_old.aba() + 1);

    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Push, &item)) {
        return true;
      }
    } else {
      break;
    }
  }
  return true;
}

template<typename T>
bool EliminationBackoffStack<T>::pop(T *item) {
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  while (true) {
    top_old = *top_;
    if (top_old.value() == NULL) {
      return false;
    }
    top_new.weak_set_value(top_old.value()->next.value());
    top_new.weak_set_aba(top_old.aba() + 1);

    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Pop, item)) {
        return true;
      }
    } else {
      break;
    }
  }
  *item = top_old.value()->data;
  return true;
}

template<typename T>
inline bool EliminationBackoffStack<T>::get_return_empty_state(T *item, AtomicRaw *state) {
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  while (true) {
    top_old = *top_;
    if (top_old.value() == NULL) {
      *state = top_old.raw();
      return false;
    }
    top_new.weak_set_value(top_old.value()->next.value());
    top_new.weak_set_aba(top_old.aba() + 1);
    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Pop, item)) {
        return true;
      }
    } else {
      break;
    }
  }
  *item = top_old.value()->data;
  *state = top_old.raw();
  return true;
}

#endif  // SCAL_DATASTRUCTURES_ELIMINATION_BACKOFF_STACK_H_
