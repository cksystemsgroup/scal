// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdint.h>
#include <string.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/distributed_data_structure.h"

DEFINE_uint64(p, 80, "number of partial queues");

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

#if   defined(BALANCER_1RANDOM)

#include "datastructures/balancer_1random.h"
DEFINE_bool(hw_random, false, "use hardware random generator instead "
                              "of pseudo");
#define GENERATE_BALANCER() (new scal::Balancer1Random(FLAGS_hw_random))
#define BALANCER_T() scal::Balancer1Random

#elif defined(BALANCER_LL)

#include "datastructures/balancer_local_linearizability.h"
#define GENERATE_BALANCER() (new scal::BalancerLocalLinearizability(FLAGS_p))
#define BALANCER_T() scal::BalancerLocalLinearizability

#else

#error "unknown balancer"

#endif  // BALANCER_*

void* ds_new() {
  return static_cast<void*>(
      new scal::DistributedDataStructure<T, BACKEND(), BALANCER_T() >(
          FLAGS_p, g_num_threads + 1, GENERATE_BALANCER()));
}


char* ds_get_stats() { return NULL; }
