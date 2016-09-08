// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_BALANCER_H_
#define SCAL_DATASTRUCTURES_BALANCER_H_

#include <inttypes.h>

class BalancerInterface {
 public:
  virtual uint64_t get_id() = 0;
  virtual uint64_t put_id() = 0;
  virtual bool local_get_id(uint64_t* idx) { return false; }

  virtual ~BalancerInterface() {}
};

#endif  // SCAL_DATASTRUCTURES_BALANCER_H_
