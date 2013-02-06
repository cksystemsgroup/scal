// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SRC_DATASTRUCTURES_DISTRIBUTED_QUEUE_H_
#define SRC_DATASTRUCTURES_DISTRIBUTED_QUEUE_H_

#include <stdint.h>

#include "datastructures/balancer.h"
#include "datastructures/distributed_queue_interface.h"
#include "datastructures/ms_queue.h"
#include "datastructures/pool.h"
#include "util/malloc.h"
#include "util/platform.h"

template<typename T, class P>
class DistributedQueue : public Pool<T> {
 public:
  DistributedQueue(size_t num_queues,
           uint64_t num_threads,
           BalancerInterface *balancer);
  bool put(T item);
  bool get(T *item);

 private:
  static const uint64_t kPtrAlignment = scal::kCachePrefetch;

  P **backend_;
  size_t num_queues_;
  BalancerInterface *balancer_;
  AtomicRaw **tails_;
};

template<typename T, class P>
DistributedQueue<T, P>::DistributedQueue(
    size_t num_queues, uint64_t num_threads, BalancerInterface *balancer) {
  num_queues_ = num_queues;
  balancer_ = balancer;
  backend_ = static_cast<P**>(calloc(num_queues_, sizeof(P*)));
  for (uint64_t i = 0; i < num_queues_; i++) {
    backend_[i] = scal::get<P>(kPtrAlignment);
  }
  tails_ = static_cast<AtomicRaw**>(calloc(num_threads, sizeof(*tails_)));
  for (uint64_t i = 0; i < num_threads; i++) {
    tails_[i] = static_cast<AtomicRaw*>(scal::tlcalloc_aligned(
        num_queues_, sizeof(AtomicRaw), kPtrAlignment));
  }
}

template<typename T, class P>
bool DistributedQueue<T, P>::put(T item) {
  uint64_t index = balancer_->get(num_queues_, NULL, true);
  return backend_[index]->put(item);
}

template<typename T, class P>
bool DistributedQueue<T, P>::get(T *item) {
  size_t i;
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  uint64_t start = balancer_->get(num_queues_, NULL, false);
  size_t index;
  while (true) {
    for (i = 0; i < num_queues_; i++) {
      index = (start + i) % num_queues_;
      if (backend_[index]->get_return_empty_state(
              item, &(tails_[thread_id][index]))) {
        return true;
      }
    }
    for (i = 0; i < num_queues_; i++) {
      index = (start + i) % num_queues_;
      if (backend_[index]->empty_state() != tails_[thread_id][index]) {
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
