// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/flatcombining_queue.h"

DEFINE_uint64(array_size, 100, "operations array size");

void* ds_new() {
  FlatCombiningQueue<uint64_t> *fcq =
      new FlatCombiningQueue<uint64_t>(FLAGS_array_size);
  return static_cast<void*>(fcq);
}

char* ds_get_stats(void) {
  return NULL;
}
