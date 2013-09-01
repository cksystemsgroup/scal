// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ts_timestamp.h"
#include "datastructures/ts_deque.h"

DEFINE_bool(list, false, "use the linked-list-based inner buffer");
DEFINE_bool(2ts, false, "use the 2-time-stamp inner buffer");
DEFINE_bool(stutter_clock, false, "use the stuttering clock");
DEFINE_bool(atomic_clock, false, "use atomic fetch-and-inc clock");
DEFINE_bool(hw_clock, false, "use the RDTSC hardware clock");
DEFINE_bool(init_threshold, false, "initializes the dequeue threshold "
    "with the current time");
DEFINE_uint64(delay, 0, "delay in the insert operation");
DEFINE_bool(insert_left, false, "put is mapped to insert_left");
DEFINE_bool(insert_right, false, "put is mapped to insert_right");
DEFINE_bool(insert_random, false, "put is mapped randomly to insert_left "
    " and insert_right");
DEFINE_bool(remove_left, false, "put is mapped to remove_left");
DEFINE_bool(remove_right, false, "put is mapped to remove_right");
DEFINE_bool(remove_random, false, "put is mapped randomly to remove_left "
    " and remove_right");

void* ds_new() {
  uint64_t insert_side = DequeSide::kRight;
  uint64_t remove_side = DequeSide::kRight;

  if (FLAGS_insert_left) {
    insert_side = DequeSide::kLeft;
  } else if (FLAGS_insert_random) {
    insert_side = DequeSide::kRandom;
  }

  if (FLAGS_remove_left) {
    remove_side = DequeSide::kLeft;
  } else if (FLAGS_remove_random) {
    remove_side = DequeSide::kRandom;
  }

  TimeStamp *timestamping;
  if (FLAGS_stutter_clock) {
    timestamping = new StutteringTimeStamp(g_num_threads + 1);
  } else if (FLAGS_atomic_clock) {
    timestamping = new AtomicCounterTimeStamp();
  } else if (FLAGS_hw_clock) {
    timestamping = new ShiftedHardwareTimeStamp();
  } else {
    timestamping = new ShiftedHardwareTimeStamp();
  }
  TSDequeBuffer<uint64_t> *buffer;
  if (FLAGS_list) {
    buffer = new TLLinkedListDequeBuffer<uint64_t>(g_num_threads + 1);
  } else if (FLAGS_2ts) {
    buffer 
      = new TL2TSDequeBuffer<uint64_t>(g_num_threads + 1, FLAGS_delay);
  } else {
    buffer = new TLLinkedListDequeBuffer<uint64_t>(g_num_threads + 1);
  }
  TSDeque<uint64_t> *ts =
      new TSDeque<uint64_t>(buffer, timestamping, FLAGS_init_threshold,
          insert_side, remove_side);
  return static_cast<void*>(ts);
}

char* ds_get_stats(void) {
  return NULL;
}
