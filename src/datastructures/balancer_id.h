// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_ID_H_
#define SCAL_DATASTRUCTURES_BALANCER_ID_H_

#include "datastructures/balancer.h"
#include "util/threadlocals.h"

class BalancerId : public BalancerInterface {
 public:
  BalancerId() { }

  uint64_t get(uint64_t num_queues, MSQueue<uint64_t> **queues, bool enqueue) {
    if (num_queues == 1) {
      return 0;
    }
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    return thread_id % num_queues;
  }
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_ID_H_
