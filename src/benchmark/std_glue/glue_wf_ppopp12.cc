// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/wf_queue_ppopp12.h"

DEFINE_uint64(max_retries, 10, "maximum number of retries in the fast path");
DEFINE_uint64(helping_delay, 10, "number of iterations helping is derfered");

void* ds_new(void) {
  // Main thread may also need the queue.
  WaitfreeQueue<uint64_t> *wfq = new WaitfreeQueue<uint64_t>(
      g_num_threads + 1, FLAGS_max_retries, FLAGS_helping_delay);
  return static_cast<void*>(wfq);
}

char* ds_get_stats(void) {
  return NULL;
}
