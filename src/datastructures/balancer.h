// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_H_
#define SCAL_DATASTRUCTURES_BALANCER_H_

#include <stdint.h>

#include "datastructures/ms_queue.h"

class BalancerInterface {
 public:
  virtual uint64_t get(uint64_t num_queues,
                       MSQueue<uint64_t> **queues,
                       bool enqueue) = 0;
  virtual ~BalancerInterface() {}
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_H_
