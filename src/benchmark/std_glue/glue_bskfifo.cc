// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/boundedsize_kfifo.h"

DEFINE_uint64(k, 80, "k-segment size");
DEFINE_uint64(num_segments, 100000, "number of k-segments in the "
                                     "bounded-size version");

void* ds_new() {
  return static_cast<void*>(
      new scal::BoundedSizeKFifo<uint64_t>(FLAGS_k, FLAGS_num_segments));
}


char* ds_get_stats(void) {
  return NULL;
}
