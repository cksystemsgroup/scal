// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.


#ifndef SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_
#define SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_

#include <atomic>

#include "datastructures/treiber_stack.h"
#include "datastructures/pool.h"
#include "util/random.h"

namespace scal {

template<typename T>
class LRUDistributedStack : public Pool<T> {
 public:
  LRUDistributedStack(uint64_t p);

  bool push(T item);
  bool pop(T *item);

 private:
  uint64_t p_;
  TreiberStack<T>* backends_;
  uint8_t padding1[128];      // TODO why is padding required?
  std::atomic<uint64_t> op_counter_;
  uint8_t padding2[128];      // TODO why is padding required?
};

template<typename T>
LRUDistributedStack<T>::LRUDistributedStack(uint64_t p)
    : p_(p)
    , backends_(new TreiberStack<T>[p]) {
  op_counter_.store(0);
}


template<typename T>
bool LRUDistributedStack<T>::push(T item) {
  const uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  while (true) {
    // TODO: check if this is correct
    lowest_tag = op_counter_.load() / p_;
    // _i .. ensure to check each stack only once
    // i ... index of stack (begin at start and wrap around at p_-1)
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if (backends_[i].try_push(item, lowest_tag)) {
        // fetch_add is a CPU instruction that atomically increments
        // the value of a memory location and returns the old value
        uint64_t old = op_counter_.fetch_add(1);
        // 65535 max. tag value (uint16_t)
        // op_counter_ can be greater than max. tag value by p_-1 
        if (old == ((65535 * p_) + (p_-1))) {
          put_counter_.store(0);  
        }
        return true;
      }
    }
  }
  return false;  // UNREACHABLE

}

template<typename T>
bool LRUDistributedStack<T>::pop(T* item) {
  uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  State tops[p_];  // NOLINT
  while (true) {
    lowest_tag = op_counter_.load() / p_;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      uint64_t old;
      switch(backends_[i].try_pop(item, lowest_tag, &tops[i])) {
        case 0:  // ok
          old = op_counter_.fetch_add(1);
          if (old == ((65535 * p_) + (p_-1))) {
            get_counter_.store(0);  
          }
          return true;
          break;
        case 1:  // empty
          break;
        case 2:  // head changed or tag not matching
          tops[i] = 0;
          break;
        default:
          fprintf(stderr, "unknown return value\n");
          abort();
      }
    }
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if ((tops[i] != 0) && (backends_[i].put_state() != tops[i])) {
        start = i;
        break;
      }
      if (_i == (p_ - 1)) {
        return false;
      }
    } 
  }
  return false;  // UNREACHABLE
}


}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_
