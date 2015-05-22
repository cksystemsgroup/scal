// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdint.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/dyn_distributed_data_structure.h"

#define T uint64_t

#if   defined(BACKEND_MS_QUEUE)

#include "datastructures/ms_queue.h"
#define BACKEND() scal::MSQueue<uint64_t>

#elif defined(BACKEND_TREIBER)

#include "datastructures/treiber_stack.h"
#define BACKEND() scal::TreiberStack<uint64_t>

#else

#error "unknown backend"

#endif  // BACKEND_*

void* ds_new() {
  return static_cast<void*>(
      new scal::DynamicDistributedDataStructure<T, BACKEND() >(1024));
}


char* ds_get_stats() { return NULL; }
