// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/flatcombining_deque.h"

DEFINE_uint64(array_size, 0, "operations array size");

void* ds_new() {
  FlatCombiningDeque<uint64_t> *fcd;
  if (FLAGS_array_size != 0) {
    fcd = new FlatCombiningDeque<uint64_t>(FLAGS_array_size);
  } else {
    fcd = new FlatCombiningDeque<uint64_t>(g_num_threads + 1);
  }
  return static_cast<void*>(fcd);
}

char* ds_get_stats(void) {
  return NULL;
}
