// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/lockbased_stack.h"

void* ds_new(void) {
  return static_cast<void*>(new scal::LockBasedStack<uint64_t>());
}


char* ds_get_stats(void) {
  return NULL;
}
