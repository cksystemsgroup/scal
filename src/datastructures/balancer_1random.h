// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES__BALANCER_1RANDOM_H_
#define SCAL_DATASTRUCTURES__BALANCER_1RANDOM_H_

#include "datastructures/balancer.h"

#include "util/platform.h"
#include "util/random.h"

namespace scal {

class Balancer1Random {
 public:
  _always_inline explicit Balancer1Random(uint64_t size, bool use_hw_random) 
      : size_(size), use_hw_random_(use_hw_random) {
  }

  _always_inline uint64_t get_id() {
    return scal::pseudorand() % size_;
  }

  _always_inline uint64_t put_id() {
    return scal::pseudorand() % size_;
  }

 private:
  uint64_t size_;
  bool use_hw_random_;
};

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_BALANCER_1RANDOM_H_
