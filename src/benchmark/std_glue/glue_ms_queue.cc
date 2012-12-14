// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/ms_queue.h"

void* ds_new(void) {
  MSQueue<uint64_t> *msq = new MSQueue<uint64_t>();
  return static_cast<void*>(msq);
}

bool ds_put(void *ds, uint64_t item) {
  MSQueue<uint64_t> *msq = static_cast<MSQueue<uint64_t>*>(ds);
  return msq->enqueue(item);
}

bool ds_get(void *ds, uint64_t *item) {
  MSQueue<uint64_t> *msq = static_cast<MSQueue<uint64_t>*>(ds);
  return msq->dequeue(item);
}

char* ds_get_stats(void) {
  return NULL;
}
