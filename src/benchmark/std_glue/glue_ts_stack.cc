// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ts_stack.h"

void* ds_new() {
  HardwareTimeStamp *timestamping = new HardwareTimeStamp();
  TLArrayBuffer<uint64_t> *buffer 
    = new TLArrayBuffer<uint64_t>(g_num_threads + 1);
  TSStack<uint64_t> *ts =
      new TSStack<uint64_t>(buffer, timestamping);
  return static_cast<void*>(ts);
}

char* ds_get_stats(void) {
  return NULL;
}
