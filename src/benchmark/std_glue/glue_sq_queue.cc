// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/segment_queue.h"

DEFINE_uint64(quasi_factor, 80, "random dequeue quasi factor");

void* ds_new(void) {
  return static_cast<void*>(
      new scal::SegmentQueue<uint64_t>(FLAGS_quasi_factor));
}


char* ds_get_stats(void) {
  return NULL;
}
