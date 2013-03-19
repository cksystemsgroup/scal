// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends

#ifndef SCAL_DATASTRUCTURES__BALANCER_2RANDOM_H_
#define SCAL_DATASTRUCTURES__BALANCER_2RANDOM_H_

#include "datastructures/partial_pool_interface.h"
#include "datastructures/balancer.h"
#include "datastructures/ms_queue.h"
#include "util/random.h"
#include <stdint.h>

class Balancer2Random : public BalancerInterface {
 public:
  explicit Balancer2Random(bool use_hw_random) {
    use_hw_random_ = use_hw_random;
  }

  uint64_t get(uint64_t num_queues, PartialPoolInterface **queues, bool enqueue) {
    if (num_queues == 1) {
      return 0;
    }
    
    uint64_t index1;
    uint64_t index2;

    if (use_hw_random_) {
      index1 = hwrand() % num_queues;
      index2 = hwrand() % num_queues;
    } else {
      index1 = pseudorand() % num_queues;
      index2 = pseudorand() % num_queues;
    }

    uint64_t size1 = queues[index1]->approx_size();
    uint64_t size2 = queues[index2]->approx_size();
//    uint64_t size1 = 10; 
//    uint64_t size2 = 12;

    if ((enqueue && (size1 < size2)) ||
       !(!enqueue && (size1 > size2))) {
      return index1;
    }
    return index2;
  }

 private:
  bool use_hw_random_;
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_2RANDOM_H_
