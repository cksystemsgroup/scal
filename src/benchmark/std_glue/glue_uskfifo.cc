// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdio.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/unboundedsize_kfifo.h"

DEFINE_uint64(k, 80, "k-segment size");
DEFINE_uint64(num_segments, 1000000, "number of k-segments in the bounded-size version");

void* ds_new() {
  UnboundedSizeKFifo<uint64_t> *kfifo =
      new UnboundedSizeKFifo<uint64_t>(FLAGS_k);
  return static_cast<void*>(kfifo);
}

bool ds_put(void *ds, uint64_t item) {
  UnboundedSizeKFifo<uint64_t> *kfifo =
      static_cast<UnboundedSizeKFifo<uint64_t>*>(ds);
  return kfifo->enqueue(item);
}

bool ds_get(void *ds, uint64_t *item) {
  UnboundedSizeKFifo<uint64_t> *kfifo =
      static_cast<UnboundedSizeKFifo<uint64_t>*>(ds);
  return kfifo->dequeue(item);
}

char* ds_get_stats(void) {
  return NULL;
}
