// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/wf_queue.h"

void* ds_new(void) {
  // Main thread may also need the queue.
  WaitfreeQueue<uint64_t> *wfq = new WaitfreeQueue<uint64_t>(g_num_threads + 1);
  return static_cast<void*>(wfq);
}

bool ds_put(void *ds, uint64_t item) {
  WaitfreeQueue<uint64_t> *wfq = static_cast<WaitfreeQueue<uint64_t>*>(ds);
  return wfq->enqueue(item);
}

bool ds_get(void *ds, uint64_t *item) {
  WaitfreeQueue<uint64_t> *wfq = static_cast<WaitfreeQueue<uint64_t>*>(ds);
  return wfq->dequeue(item);
}

char* ds_get_stats(void) {
  return NULL;
}
