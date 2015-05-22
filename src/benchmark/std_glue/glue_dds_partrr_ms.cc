// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends

#include <gflags/gflags.h>
#include <stdint.h>
#include <string.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/balancer_partrr.h"
#include "datastructures/distributed_data_structure.h"
#include "datastructures/ms_queue.h"

DEFINE_uint64(p, 80, "number of partial queues");
DEFINE_uint64(partitions, 1, "number of round robin partitions");

void* ds_new(void) {
  return static_cast<void*>(
      new scal::DistributedDataStructure<uint64_t, scal::MSQueue<uint64_t>, scal::BalancerPartitionedRoundRobin>(
          FLAGS_p,
          g_num_threads + 1,
          new scal::BalancerPartitionedRoundRobin(FLAGS_partitions, FLAGS_p)));
}

char* ds_get_stats(void) {
  char buffer[255] = { 0 };
  uint32_t n = snprintf(buffer,
                        sizeof(buffer),
                        " ,\"p\": %" PRIu64 " ,\"partitions\":%" PRIu64,
                        FLAGS_p,
                        FLAGS_partitions);
  if (n != strlen(buffer)) {
    fprintf(stderr, "%s: error creating stats string\n", __func__);
    abort();
  }
  char *newbuf = static_cast<char*>(calloc(
      strlen(buffer) + 1, sizeof(*newbuf)));
  return strncpy(newbuf, buffer, strlen(buffer));
}
