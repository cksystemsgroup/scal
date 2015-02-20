// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_
#define SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_

#include <gflags/gflags.h>

#include "datastructures/balancer.h"
#include "util/platform.h"
#include "util/threadlocals.h"
#include "util/random.h"

DEFINE_uint64(ll_balancer_seed, 0, "local linearizability balancer seed");

class BalancerLocalLinearizability {
 public:
  BalancerLocalLinearizability(size_t size) : size_(size) {
    distribution_ = new size_t[size];
    for (size_t i = 0; i < size_; i++) {
      distribution_[i] = i;
    }
    scal::shuffle<size_t>(distribution_, size, FLAGS_ll_balancer_seed);
  }

  always_inline uint64_t get_id() {
    return scal::hwrand() % size_;
  }

  always_inline uint64_t put_id() {
    return distribution_[scal::ThreadContext::get().thread_id() % size_];
  }

  always_inline bool local_get_id(uint64_t* idx) {
    *idx = distribution_[scal::ThreadContext::get().thread_id() % size_];
    return true;
  }

 private:
  size_t size_;
  size_t* distribution_;
  uint8_t pad_[
    128
        - sizeof(size_)
        - sizeof(distribution_)];
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_LOCAL_LINEARIZABILITY_H_
