// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/dts_queue.h"

#define TS_DS DTSQueue<uint64_t>

TS_DS *ts_;

void* ds_new() {
  ts_ = new TS_DS();
  ts_->initialize(g_num_threads + 1);

  return static_cast<void*>(ts_);
}

char* ds_get_stats(void) {
  return ts_->ds_get_stats();
}
