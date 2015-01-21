// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_H_
#define SCAL_DATASTRUCTURES_BALANCER_H_

#include <stdint.h>

#include "datastructures/ms_queue.h"

class BalancerInterface {
 public:
  virtual uint64_t get(uint64_t num_queues,
                       scal::MSQueue<uint64_t> **queues,
                       bool enqueue) = 0;
  virtual ~BalancerInterface() {}
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_H_
