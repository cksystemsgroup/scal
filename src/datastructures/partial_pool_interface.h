// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_PARTIAL_POOL_INTERFACE_H_
#define SCAL_PARTIAL_POOL_INTERFACE_H_

#include <stdint.h>

#include "util/atomic_value.h"

class PartialPoolInterface {
 public:

  // Returns an approximate size of the pool. 
  virtual uint64_t approx_size(void) const = 0;
};

#endif  // SCAL_PARTIAL_POOL_INTERFACE_H_
