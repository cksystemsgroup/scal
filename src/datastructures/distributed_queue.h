// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SRC_DATASTRUCTURES_DISTRIBUTED_QUEUE_H_
#define SRC_DATASTRUCTURES_DISTRIBUTED_QUEUE_H_

#include <stdint.h>

#include "datastructures/balancer.h"
#include "datastructures/ms_queue.h"
#include "util/malloc.h"
#include "util/platform.h"

template<typename T>
class DistributedQueue {
 public:
  DistributedQueue(size_t num_queues,
           uint64_t num_threads,
           BalancerInterface *balancer);
  bool put(T item);
  bool get(T *item);

 private:
  static const uint64_t kPtrAlignment = scal::kCachePrefetch;

  MSQueue<T> **queues_;
  size_t num_queues_;
  BalancerInterface *balancer_;
  AtomicRaw **tails_;
};

template<typename T>
DistributedQueue<T>::DistributedQueue(
    size_t num_queues, uint64_t num_threads, BalancerInterface *balancer) {
  num_queues_ = num_queues;
  balancer_ = balancer;
  queues_ = static_cast<MSQueue<T>**>(calloc(num_queues_, sizeof(MSQueue<T>*)));
  for (uint64_t i = 0; i < num_queues_; i++) {
    queues_[i] = scal::get<MSQueue<T>>(kPtrAlignment);
  }
  tails_ = static_cast<AtomicRaw**>(calloc(num_threads, sizeof(AtomicRaw*)));
  for (uint64_t i = 0; i < num_threads; i++) {
    tails_[i] = static_cast<AtomicRaw*>(scal::tlcalloc_aligned(
        num_queues_, sizeof(AtomicRaw), kPtrAlignment));
  }
}

template<typename T>
bool DistributedQueue<T>::put(T item) {
  uint64_t index = balancer_->get(num_queues_, queues_, true);
  queues_[index]->enqueue(item);
  return true;  // ms queue is unbounded size, hence no full state
}

template<typename T>
bool DistributedQueue<T>::get(T *item) {
  using scal::tlcalloc_aligned;
  size_t i;
  uint64_t thread_id = threadlocals_get()->thread_id;
  uint64_t start = balancer_->get(num_queues_, queues_, false);
  uint64_t index;
  while (true) {
    for (i = 0; i < num_queues_; i++) {
      index = (start + i) % num_queues_;
      if (queues_[index]->dequeue_return_tail(
              item, &(tails_[thread_id][index]))) {
        return true;
      }
    }
    for (i = 0; i < num_queues_; i++) {
      index = (start + i) % num_queues_;
      if (queues_[index]->get_tail_raw() != tails_[thread_id][index]) {
        start = index;
        break;
      }
      if (((index + 1) % num_queues_) == start) {
        return false;
      }
    }
  }
}

#endif  // SRC_DATASTRUCTURES_DISTRIBUTED_QUEUE_H_
