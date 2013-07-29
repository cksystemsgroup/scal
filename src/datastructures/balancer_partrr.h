// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SRC_DATASTRUCTURES_BALANCER_PARTRR_H_
#define SRC_DATASTRUCTURES_BALANCER_PARTRR_H_

#include "datastructures/partial_pool_interface.h"
#include "datastructures/balancer.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/threadlocals.h"

class BalancerPartitionedRoundRobin : public BalancerInterface {
 public:
  BalancerPartitionedRoundRobin(uint64_t partitions, uint64_t num_queues) {
    num_queues_ = num_queues;
    partitions_ = partitions;
    enqueue_rrs_ = static_cast<uint64_t**>(calloc(
        partitions_, sizeof(*enqueue_rrs_)));
    dequeue_rrs_ = static_cast<uint64_t**>(calloc(
        partitions_, sizeof(*dequeue_rrs_)));
    for (uint64_t i = 0; i < partitions_; i++) {
      enqueue_rrs_[i] = scal::get<uint64_t>(scal::kCachePrefetch);
      dequeue_rrs_[i] = scal::get<uint64_t>(scal::kCachePrefetch);
      *enqueue_rrs_[i] = (num_queues_/partitions_) * i;
      *dequeue_rrs_[i] = (num_queues_/partitions_) * i;
    }
  }

  uint64_t get(uint64_t num_queues, PartialPoolInterface **queues, bool enqueue) {
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    if (enqueue) {
      return __sync_fetch_and_add(enqueue_rrs_[thread_id % partitions_], 1)
          % num_queues;
    } else {
      return __sync_fetch_and_add(dequeue_rrs_[thread_id % partitions_], 1)
          % num_queues;
    }
  }

 private:
  uint64_t num_queues_;
  uint64_t partitions_;
  uint64_t **enqueue_rrs_;
  uint64_t **dequeue_rrs_;
};

#endif  // SRC_DATASTRUCTURES_BALANCER_PARTRR_H_
