// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <stdint.h>
#include <string.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/balancer_1random.h"
#include "datastructures/distributed_queue.h"
#include "datastructures/treiber_stack.h"

DEFINE_uint64(p, 80, "number of partial queues");
DEFINE_bool(hw_random, false, "use hardware random generator instead "
                              "of pseudo");

void* ds_new(void) {
  Balancer1Random *balancer = new Balancer1Random(FLAGS_hw_random);
  DistributedQueue<uint64_t, TreiberStack<uint64_t> > *sp =
      new DistributedQueue<uint64_t, TreiberStack<uint64_t> >(FLAGS_p, g_num_threads + 1, balancer);
  return static_cast<void*>(sp);
}

bool ds_put(void *ds, uint64_t item) {
  DistributedQueue<uint64_t, TreiberStack<uint64_t> > *sp = static_cast<DistributedQueue<uint64_t, TreiberStack<uint64_t> >*>(ds);
  return sp->put(item);
}

bool ds_get(void *ds, uint64_t *item) {
  DistributedQueue<uint64_t, TreiberStack<uint64_t> > *sp = static_cast<DistributedQueue<uint64_t, TreiberStack<uint64_t> >*>(ds);
  return sp->get(item);
}

char* ds_get_stats(void) {
  return NULL;
}
