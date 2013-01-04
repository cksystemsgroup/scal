// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/lockbased_queue.h"

DEFINE_uint64(dequeue_mode, 0, "different APIs for empty dequeue: non-blocking (0), blocking (1), timeout (2)");
DEFINE_uint64(dequeue_timeout, 100, "dequeue timeout in ms");

void* ds_new(void) {
  LockBasedQueue<uint64_t> *lbq = new LockBasedQueue<uint64_t>();
  return static_cast<void*>(lbq);
}

bool ds_put(void *ds, uint64_t item) {
  LockBasedQueue<uint64_t> *lbq = static_cast<LockBasedQueue<uint64_t>*>(ds);
  return lbq->enqueue(item);
}

bool ds_get(void *ds, uint64_t *item) {
  LockBasedQueue<uint64_t> *lbq = static_cast<LockBasedQueue<uint64_t>*>(ds);
  switch (FLAGS_dequeue_mode) {
    case 0:
      return lbq->dequeue(item);
    case 1:
      return lbq->dequeue_blocking(item);
    case 2:
      return lbq->dequeue_timeout(item, FLAGS_dequeue_timeout);
    default:
      return lbq->dequeue(item);
  }
}

char* ds_get_stats(void) {
  return NULL;
}
