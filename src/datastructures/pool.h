// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_POOL_H_
#define SCAL_DATASTRUCTURES_POOL_H_

#include "util/atomic_value_new.h"

typedef uint64_t State;

template<typename T>
class Pool {
 public:
  virtual bool put(T item) = 0;
  virtual bool get(T *item) = 0;

  virtual void Terminate() {}

  virtual ~Pool() {}
};

#endif  // SCAL_DATASTRUCTURES_POOL_H_
