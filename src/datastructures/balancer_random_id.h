// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_RANDOM_ID_H_
#define SCAL_DATASTRUCTURES_BALANCER_RANDOM_ID_H_

#include "datastructures/partial_pool_interface.h"
#include "datastructures/balancer.h"
#include "datastructures/balancer_random_id.h"
#include "util/threadlocals.h"

class BalancerRandomId : public BalancerInterface {
 public:
  BalancerRandomId (bool use_hw_random) {
    use_hw_random_ = use_hw_random;
  }

  uint64_t get(uint64_t num_queues, PartialPoolInterface **queues, bool enqueue) {
    if (num_queues == 1) {
      return 0;
    }
    if (enqueue) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      return thread_id % num_queues;
    } else {
      if (use_hw_random_) {
        return hwrand() % num_queues;
      } else {
        return pseudorand() % num_queues;
      }
    }
  }

 private:
  bool use_hw_random_;
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_RANDOM_ID_H_
