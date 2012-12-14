// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES__BALANCER_1RANDOM_H_
#define SCAL_DATASTRUCTURES__BALANCER_1RANDOM_H_

#include "datastructures/balancer.h"
#include "util/random.h"

class Balancer1Random : public BalancerInterface {
 public:
  explicit Balancer1Random(bool use_hw_random) {
    use_hw_random_ = use_hw_random;
  }

  uint64_t get(uint64_t num_queues, MSQueue<uint64_t> **queues, bool enqueue) {
    if (num_queues == 1) {
      return 0;
    }
    if (use_hw_random_) {
      return hwrand() % num_queues;
    } else {
      return pseudorand() % num_queues;
    }
  }

 private:
  bool use_hw_random_;
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_1RANDOM_H_
