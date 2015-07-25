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
  explicit LRUDistributedStack(uint64_t p);

  bool put(T item);
  bool get(T *item);

 private:
  uint64_t p_;
  TreiberStack<T>* backends_;
  uint8_t padding1[128];
  std::atomic<uint64_t> put_counter_;
  uint8_t padding2[128];
  std::atomic<uint64_t> get_counter_;
  uint8_t padding3[128];
};

template<typename T>
LRUDistributedStack<T>::LRUDistributedStack(uint64_t p)
    : p_(p)
    , backends_(new TreiberStack<T>[p]) {
  put_counter_.store(0);
  get_counter_.store(0);
}

template<typename T>
bool LRUDistributedStack<T>::put(T item) {
  const uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  while (true) {
    lowest_tag = put_counter_.load() / p_;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if (backends_[i].try_push(item, lowest_tag)) {
        uint64_t old = put_counter_.fetch_add(1);
        if (old == ((255 * p_) + (p_-1))) {
          put_counter_.store(0);
        }
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
  while (true) {
    lowest_tag = get_counter_.load() / p_;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      uint64_t old;
      switch (backends_[i].try_pop(item, lowest_tag, &tops[i])) {
        case 0:  // ok
          old = get_counter_.fetch_add(1);
          if (old == ((255 * p_) + (p_-1))) {
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
