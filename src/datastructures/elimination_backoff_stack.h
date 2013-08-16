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
#include <assert.h>
#include <atomic>

#include "datastructures/distributed_queue_interface.h"
#include "datastructures/partial_pool_interface.h"
#include "datastructures/stack.h"
#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/threadlocals.h"
#include "util/random.h"

#define EMPTY 0

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

  char* ds_get_stats(void) {

    int64_t sum1 = 0;
    int64_t sum2 = 0;

    for (int i = 0; i < num_threads_; i++) {
      sum1 += *counter1_[i];
      sum2 += *counter2_[i];
    }

    char buffer[255] = { 0 };
    uint32_t n = snprintf(buffer,
                          sizeof(buffer),
                          "%ld %ld %ld",
                          sum1, sum2, sum2 / sum1
                          );
    if (n != strlen(buffer)) {
      fprintf(stderr, "%s: error creating stats string\n", __func__);
      abort();
    }
    char *newbuf = static_cast<char*>(calloc(
        strlen(buffer) + 1, sizeof(*newbuf)));
    return strncpy(newbuf, buffer, strlen(buffer));
  }

 private:
  typedef ebs_internal::Node<T> Node;
  typedef ebs_internal::Opcode Opcode;
  typedef ebs_internal::Operation<T> Operation;

  AtomicPointer<Node*> *top_;
  Operation* *operations_;

  std::atomic<uint64_t>* *location_;
  std::atomic<uint64_t>* *collision_;
  const uint64_t size_collision_;
  const uint64_t num_threads_;
  int64_t* *counter1_;
  int64_t* *counter2_;

  bool try_collision(uint64_t thread_id, uint64_t other, T *item);
  bool backoff(Opcode opcode, T *item);

  inline void inc_counter1() {
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    (*counter1_[thread_id])++;
  }

  inline void inc_counter2() {
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    (*counter2_[thread_id])++;
  }
};

template<typename T>
EliminationBackoffStack<T>::EliminationBackoffStack(
    uint64_t num_threads, uint64_t size_collision) 
  : size_collision_(size_collision), num_threads_(num_threads) {
  top_ = scal::get<AtomicPointer<Node*> >(scal::kCachePrefetch);

  operations_ = static_cast<Operation**>(
      scal::calloc_aligned(num_threads, 
          sizeof(Operation*), scal::kCachePrefetch * 2));

  location_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads, 
          sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

  collision_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(size_collision_,
          sizeof(AtomicValue<uint64_t>*), scal::kCachePrefetch * 2));
  
  counter1_ = static_cast<int64_t**>(
      scal::calloc_aligned(num_threads, sizeof(uint64_t*), 
        scal::kCachePrefetch * 2));

  counter2_ = static_cast<int64_t**>(
      scal::calloc_aligned(num_threads, sizeof(uint64_t*), 
        scal::kCachePrefetch * 2));

  for (uint64_t i = 0; i < num_threads; i++) {
    operations_[i] = scal::get<Operation>(scal::kCachePrefetch * 2);
  }
  for (uint64_t i = 0; i < num_threads; i++) {
    location_[i] = 
      scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch * 2);
  }

  for (uint64_t i = 0; i < size_collision_; i++) {
    collision_[i] = 
      scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch * 2);
  }

  for (uint64_t i = 0; i < num_threads; i++) {
    counter1_[i] = scal::get<int64_t>(scal::kCachePrefetch * 2);
    *(counter1_[i]) = 0;
    counter2_[i] = scal::get<int64_t>(scal::kCachePrefetch * 2);
    *(counter2_[i]) = 0;
  }
}

inline uint64_t get_pos(uint64_t size) {
  return hwrand() % size;
}


template<typename T>
bool EliminationBackoffStack<T>::try_collision(
    uint64_t thread_id, uint64_t other, T *item) {
  
  AtomicValue<T> old_value(other, 0);

  if (operations_[thread_id]->opcode == Opcode::Push) {


    AtomicValue<T> new_value(thread_id, 0);
    if (location_[other]->compare_exchange_weak(
          other, thread_id)) {
      return true;
    } else {
      return false;
    }
  } else {

    AtomicValue<T> new_value(EMPTY, 0);
    if (location_[other]->compare_exchange_weak(
          other, EMPTY)) {
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
  location_[thread_id]->store(thread_id);
  uint64_t position = get_pos(size_collision_);
  uint64_t him = collision_[position]->load();
  while (!collision_[position]->compare_exchange_weak(
        him, thread_id)) { 
  }
  if (him != EMPTY) {
    uint64_t other = location_[him]->load();
    if (other != 0 && 
        other == him && 
        operations_[other]->opcode != opcode) {
      uint64_t expected = thread_id;
      if (location_[thread_id]->compare_exchange_weak(
            expected, EMPTY)) {
        if (try_collision(thread_id, other, item)) {
         return true;
        } else {
          return false;
        }
      } else {
        if (opcode == Opcode::Pop) {
          *item = operations_[location_[thread_id]->load()]->data;
          location_[thread_id]->store(0);
        }
        return true;
      }
    }
  }

  // delay
  //
  
  uint64_t expected = thread_id;
  if (!location_[thread_id]->compare_exchange_strong(
        expected, EMPTY)) {
    if (opcode == Opcode::Pop) {
      *item = operations_[location_[thread_id]->load()]->data;
      location_[thread_id]->store(EMPTY);
    }
    return true;
  }

  return false;
}
template<typename T>
bool EliminationBackoffStack<T>::push(T item) {
  inc_counter1();
  Node *n = scal::tlget<Node>(0);
  n->data = item;
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  top_new.weak_set_value(n);
  while (true) {
    inc_counter2();
    top_old = *top_;
    n->next.weak_set_value(top_old.value());
    top_new.weak_set_aba(top_old.aba() + 1);

    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Push, &item)) {
        inc_counter2();
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
  inc_counter1();
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  while (true) {
    inc_counter2();
    top_old = *top_;
    if (top_old.value() == NULL) {
      return false;
    }
    top_new.weak_set_value(top_old.value()->next.value());
    top_new.weak_set_aba(top_old.aba() + 1);

    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Pop, item)) {
        inc_counter2();
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
inline bool EliminationBackoffStack<T>::get_return_empty_state(
    T *item, AtomicRaw *state) {
  inc_counter1();
  AtomicPointer<Node*> top_old;
  AtomicPointer<Node*> top_new;
  while (true) {
    inc_counter2();
    top_old = *top_;
    if (top_old.value() == NULL) {
      *state = top_old.raw();
      return false;
    }
    top_new.weak_set_value(top_old.value()->next.value());
    top_new.weak_set_aba(top_old.aba() + 1);
    if (!top_->cas(top_old, top_new)) {
      if (backoff(Opcode::Pop, item)) {
        inc_counter2();
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
