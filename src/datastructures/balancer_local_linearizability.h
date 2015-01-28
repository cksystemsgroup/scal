// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_
#define SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_

#include "datastructures/balancer.h"
#include "util/threadlocals.h"
#include "util/random.h"

class BalancerLocalLinearizability : public BalancerInterface {
 public:
  BalancerLocalLinearizability() : max_(1) { }

  uint64_t get(uint64_t num_queues, bool enqueue) {
    if (num_queues == 1) {
      return 0;
    }
    uint64_t id;
    if (enqueue) {
      id = reinterpret_cast<uint64_t>(scal::ThreadContext::get().get_data());
      if (id == 0) {
        id = scal::ThreadContext::get().thread_id();
        max_++;
        scal::ThreadContext::get().set_data(reinterpret_cast<void*>(id));
      }
    } else {
      id = hwrand();
    }
    return (id % max_) % num_queues;
  }

 private:
  uint8_t max_;
  uint8_t pad_[63];
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_
