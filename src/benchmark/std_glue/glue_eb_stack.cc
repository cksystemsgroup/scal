// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/elimination_backoff_stack.h"

void* ds_new() {
  EliminationBackoffStack<uint64_t> *ebs =
      new EliminationBackoffStack<uint64_t>(g_num_threads + 1, (g_num_threads + 1) / 2 );
  return static_cast<void*>(ebs);
}

char* ds_get_stats(void) {
  return NULL;
}
