// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdio.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/kstack.h"

DEFINE_uint64(k, 80, "k-segment size");

void* ds_new() {
  return static_cast<void*>(
      new scal::KStack<uint64_t>(FLAGS_k, g_num_threads + 1));
}


char* ds_get_stats(void) {
  return NULL;
}
