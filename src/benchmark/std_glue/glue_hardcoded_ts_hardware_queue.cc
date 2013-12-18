// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ts_timestamp.h"
#include "datastructures/ts_queue_buffer.h"
#include "datastructures/ts_queue.h"

DEFINE_uint64(delay, 0, "delay in the insert operation");

#define TS_DS TSQueue<uint64_t, TSQueueBuffer<uint64_t, HardwareTimestamp>, HardwareTimestamp>

TS_DS *ts_;

void* ds_new() {
  ts_ = new TS_DS(g_num_threads + 1, FLAGS_delay);
  return static_cast<void*>(ts_);
}

char* ds_get_stats(void) {
  return ts_->ds_get_stats();
}
