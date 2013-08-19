// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ts_stack.h"

DEFINE_bool(array, false, "use the array-based inner buffer");
DEFINE_bool(list, false, "use the linked-list-based inner buffer");
DEFINE_bool(stutter_clock, false, "use the stuttering clock");
DEFINE_bool(atomic_clock, false, "use atomic fetch-and-inc clock");
DEFINE_bool(hw_clock, false, "use the RDTSC hardware clock");
DEFINE_bool(init_threshold, false, "initializes the dequeue threshold "
    "with the current time");

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
  TSBuffer<uint64_t> *buffer;
  if (FLAGS_array) {
    buffer = new TLArrayBuffer<uint64_t>(g_num_threads + 1);
  } else if (FLAGS_list) {
    buffer = new TLLinkedListBuffer<uint64_t>(g_num_threads + 1);
  } else {
    buffer = new TLLinkedListBuffer<uint64_t>(g_num_threads + 1);
  }
  TSStack<uint64_t> *ts =
      new TSStack<uint64_t>(buffer, timestamping, FLAGS_init_threshold);
  return static_cast<void*>(ts);
}

char* ds_get_stats(void) {
  return NULL;
}
