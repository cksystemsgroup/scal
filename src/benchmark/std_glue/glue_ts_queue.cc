// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ts_timestamp.h"
#include "datastructures/ts_queue.h"

DEFINE_bool(2ts, false, "use the array-based inner buffer");
DEFINE_bool(list, false, "use the linked-list-based inner buffer");
DEFINE_bool(stutter_clock, false, "use the stuttering clock");
DEFINE_bool(atomic_clock, false, "use atomic fetch-and-inc clock");
DEFINE_bool(hw_clock, false, "use the RDTSC hardware clock");
DEFINE_uint64(delay, 0, "delay in the insert operation");

TSQueue<uint64_t> *ts;
void* ds_new() {
  TimeStamp *timestamping;
  if (FLAGS_stutter_clock) {
    timestamping = new StutteringTimeStamp(g_num_threads + 1);
  } else if (FLAGS_atomic_clock) {
    timestamping = new AtomicCounterTimeStamp();
  } else if (FLAGS_hw_clock) {
    timestamping = new HardwareTimeStamp();
  } else {
    timestamping = new HardwareTimeStamp();
  }
  TSQueueBuffer<uint64_t> *buffer;
  if (FLAGS_2ts) {
    buffer = new TL2TSQueueBuffer<uint64_t>(g_num_threads + 1);
  } else if (FLAGS_list) {
    buffer = new TLLinkedListQueueBuffer<uint64_t>(g_num_threads + 1);
  } else {
    buffer = new TLLinkedListQueueBuffer<uint64_t>(g_num_threads + 1);
  }
  ts =
      new TSQueue<uint64_t>(buffer, timestamping, g_num_threads + 1, FLAGS_delay);
  return static_cast<void*>(ts);
}

char* ds_get_stats(void) {
  return ts->ds_get_stats();
}
