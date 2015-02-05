// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends

#include "datastructures/dyn_distributed_queue.h"
#include "datastructures/treiber_stack.h"

void* ds_new() {
    return static_cast<void*>(
        new scal::DynamicDistributedQueue<uint64_t, scal::TreiberStack<uint64_t>>(
            1024));
}


char* ds_get_stats(void) {
  return NULL;
}
