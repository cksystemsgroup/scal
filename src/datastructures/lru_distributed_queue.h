// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_QUEUE_H_
#define SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_QUEUE_H_

#include <atomic>
#include <limits>

#include "datastructures/ms_queue.h"
#include "datastructures/pool.h"
#include "util/random.h"

namespace scal {

template<typename T>
class LRUDistributedQueue : public Pool<T> {
 public:
  explicit LRUDistributedQueue(uint64_t p);

  bool put(T item);
  bool get(T* item);

 private:
  uint64_t p_;
  MSQueue<T>* backends_;
  uint8_t padding1[128];
  std::atomic<uint64_t> put_counter_;
  uint8_t padding2[128];
  std::atomic<uint64_t> get_counter_;
  uint8_t padding3[128];
};


template<typename T>
LRUDistributedQueue<T>::LRUDistributedQueue(uint64_t p)
    : p_(p)
    , backends_(new MSQueue<T>[p]) {
  put_counter_.store(0);
  get_counter_.store(0);
}


template<typename T>
bool LRUDistributedQueue<T>::put(T item) {
  const uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  while (true) {
    lowest_tag = put_counter_.load() / p_;
    // _i .. ensure to check each queue only once
    // i ... index of queue (begin at start and wrap around at p_-1)
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if (backends_[i].try_enqueue(item, lowest_tag)) {
        // fetch_add is a CPU instruction that atomically increments
        // the value of a memory location and returns the old value
        uint64_t old = put_counter_.fetch_add(1);
        // 65535 max. tag value (uint16_t)
        // put_counter can be greater than max. tag value by p_-1
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
bool LRUDistributedQueue<T>::get(T* item) {
  uint64_t start = hwrand() % p_;
  uint64_t lowest_tag;
  State tails[p_];  // NOLINT
  while (true) {
    lowest_tag = get_counter_.load() / p_;
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      uint64_t old;
      switch (backends_[i].try_dequeue(item, lowest_tag, &tails[i])) {
        case 0:  // ok
          old = get_counter_.fetch_add(1);
          if (old == ((65535 * p_) + (p_-1))) {
            get_counter_.store(0);
          }
          return true;
          break;
        case 1:  // empty
          break;
        case 2:  // head changed or tag not matching
          tails[i] = 0;
          break;
        default:
          fprintf(stderr, "unknown return value\n");
          abort();
      }
    }
    for (uint64_t _i = 0, i = start; _i < p_; _i++, i = (start + _i) % p_) {
      if ((tails[i] != 0) && (backends_[i].put_state() != tails[i])) {
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

#endif  // SCAL_DATASTRUCTURES_LRU_DISTRIBUTED_QUEUE_H_
