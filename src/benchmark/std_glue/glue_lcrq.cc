// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <stdlib.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/lcrq.h"


void* ds_new(void) {
  return static_cast<void*>(new scal::LCRQ<uint64_t>());
}


char* ds_get_stats(void) {
  return NULL;
}
