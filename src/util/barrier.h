// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/malloc.h"
#include "util/platform.h"

class SpinningBarrier {
 public:
  explicit inline SpinningBarrier(size_t num) {
    num_ = num; 
    cnt_ = scal::get<uint64_t>(scal::kCachePrefetch);
    *cnt_ = 0;
  }

  inline bool wait() {
    uint64_t old = __sync_fetch_and_add(cnt_, 1);
    // Do not simplfy num_ into the multiplication.
    uint64_t next = (old / num_) * num_ + num_;  
    while (*cnt_ < next);
    if (old == (next - 1)) {
      return true;
    }
    return false;
  }

 private:
  volatile uint64_t *cnt_;
  size_t num_;  
};
