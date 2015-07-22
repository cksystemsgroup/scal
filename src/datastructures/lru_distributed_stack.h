// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.


#ifndef SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_
#define SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_

#include <atomic>
#include <iostream>

#include "datastructures/treiber_stack.h"
#include "datastructures/pool.h"
#include "util/random.h"

namespace scal {

template<typename T>
class LRUDistributedStack : public Pool<T> {
 public:
  LRUDistributedStack(uint64_t p);

  bool put(T item);
  bool get(T *item);

 private:
  uint64_t p_;
  TreiberStack<T>* backends_;
  uint8_t padding1[128];      // TODO why is padding required?
  std::atomic<uint64_t> op_counter_;
  uint8_t padding2[128];      // TODO why is padding required?
  int x;  // TODO: remove - debugging
};

template<typename T>
LRUDistributedStack<T>::LRUDistributedStack(uint64_t p)
    : p_(p)
    , backends_(new TreiberStack<T>[p]) {
  std::cout << "init stack: " << p << std::endl;
  op_counter_.store(0);
  x = 0;
}


template<typename T>
bool LRUDistributedStack<T>::put(T item) {
  const uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  std::cout << "put" << std::endl;
  while (true) {
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
          op_counter_.store(0);
        }
        std::cout << "put success" << std::endl; //fully (op_counter=" << op_counter_.load() << ")" << std::endl;
        return true;
      }
    }
  }
  return false;  // UNREACHABLE

}

template<typename T>
bool LRUDistributedStack<T>::get(T* item) {
  uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  State tops[p_];  // NOLINT
  x++;
  if (x == 10) {
    std::cout << "abort get" << std::endl;
    abort();
  }
  std::cout << "get" << std::endl;
  while (true) {
    lowest_tag = op_counter_.load() / p_;
    //std::cout << "lowest_tag: " << lowest_tag << " op_counter: " << op_counter_.load() << std::endl;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      uint64_t old;
      switch(backends_[i].try_pop(item, lowest_tag, &tops[i])) {
        case 0:  // ok
          std::cout << "ok" << std::endl;
          old = op_counter_.fetch_add(1);
          if (old == ((65535 * p_) + (p_-1))) {
            op_counter_.store(0);
          }
          return true;
          break;
        case 1:  // empty
          std::cout << "empty" << std::endl;
          break;
        case 2:  // head changed or tag not matching
          std::cout << "head changed or tag not matching" << std::endl;
          tops[i] = 0;
          break;
        default:
          fprintf(stderr, "unknown return value\n");
          abort();
      }
    }
    std::cout << "all stacks checked once" << std::endl;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if ((tops[i] != 0) && (backends_[i].put_state() != tops[i])) {
        start = i;
        std::cout << "something changed" << std::endl;
        break;
      }
      if (_i == (p_ - 1)) {
        std::cout << "all stacks empty" << std::endl;
        return false;
      }
    }
  }
  return false;  // UNREACHABLE
}


}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_STACK_H_
