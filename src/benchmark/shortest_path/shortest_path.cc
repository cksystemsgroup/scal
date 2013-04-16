// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends

#include <gflags/gflags.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "benchmark/common.h"
#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/pool.h"
#include "util/malloc.h"
#include "util/operation_logger.h"
#include "util/random.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/workloads.h"
#include "util/platform.h"

DEFINE_string(prealloc_size, "1g", "tread local space that is initialized");
DEFINE_uint64(threads, 1, "number of threads");
DEFINE_uint64(height, 100, "height of the grid");
DEFINE_uint64(width, 10000, "width of the grid");
DEFINE_uint64(start, 1, "starting node");
DEFINE_int64(end, -1, "end node");
//DEFINE_uint64(c, 5000, "computational workload");
DEFINE_bool(print_summary, true, "print execution summary");
DEFINE_bool(log_operations, false, "log invocation/response/linearization "
                                   "of all operations");

#define CACHE_FACTOR 16 

using scal::Benchmark;

inline int get_index(int index) {
  return index * CACHE_FACTOR;
}

class ShortestPathBench : public Benchmark {
 public:
  ShortestPathBench(uint64_t num_threads,
               uint64_t thread_prealloc_size,
               void *data)
                   : Benchmark(num_threads,
                               thread_prealloc_size,
                               data) {}
 protected:
  void bench_func(void);

 private:
};

uint64_t g_num_threads;
uint64_t *g_marking;

int main(int argc, const char **argv) {
  std::string usage("Shortest path micro benchmark.");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  uint64_t tlsize = scal::human_size_to_pages(FLAGS_prealloc_size.c_str(),
                                              FLAGS_prealloc_size.size());

  // Init the main program as executing thread (may use rnd generator or tl
  // allocs).
  g_num_threads = FLAGS_threads;
  scal::tlalloc_init(tlsize, true /* touch pages */);
  //threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

 // if (FLAGS_log_operations) {
 //   scal::StdOperationLogger::prepare(g_num_threads + 1,
 //                                     FLAGS_operations +100000);
 // }

  void *ds = ds_new();
  Pool<uint64_t> *queue = static_cast<Pool<uint64_t>*>(ds);

  g_marking = static_cast<uint64_t*>(scal::malloc_aligned(
        FLAGS_height * FLAGS_width * sizeof(uint64_t) * CACHE_FACTOR, scal::kPageSize));
  memset(g_marking, 0, FLAGS_height * FLAGS_width * sizeof(uint64_t) * CACHE_FACTOR);

  g_marking[get_index(FLAGS_start)] = 1;
  queue->put(FLAGS_start);

  if (FLAGS_end < 0) {
    FLAGS_end = FLAGS_height * FLAGS_width - 1;
  }

  ShortestPathBench *benchmark = new ShortestPathBench(
      g_num_threads,
      tlsize,
      ds);
  benchmark->run();

  if (FLAGS_log_operations) {
    scal::StdOperationLogger::print_summary();
  }

  if (FLAGS_print_summary) {
    uint64_t exec_time = benchmark->execution_time();
    char buffer[1024] = {0};
    uint32_t n = snprintf(buffer, sizeof(buffer), "threads: %" PRIu64 " runtime: %" PRIu64 " costs: %" PRIu64 " ds_stats: ",
        FLAGS_threads,
        exec_time,
        g_marking[get_index(FLAGS_end)]);

    if (n != strlen(buffer)) {
      fprintf(stderr, "%s: error: failed to create summary string\n", __func__);
      abort();
    }
    char *ds_stats = ds_get_stats();
    if (ds_stats != NULL) {
      if (n + strlen(ds_stats) >= 1023) {  // separating space + '\0'
        fprintf(stderr, "%s: error: strings too long\n", __func__);
        abort();
      }
      strcat(buffer, " ");
      strcat(buffer, ds_stats);
    }
    printf("%s\n", buffer);
  }
  return EXIT_SUCCESS;
}

void ShortestPathBench::bench_func(void) {

  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  
  uint64_t node;
  bool ok;

  while (g_marking[get_index(FLAGS_end)] == 0) {

    ok = ds->get(&node);
    if (!ok) {
      continue;
    }

    int distance = g_marking[get_index(node)];

    int neighbor_above = node - FLAGS_width;
    int neighbor_below = node + FLAGS_width;
    int neighbor_right = node + 1;

    if (neighbor_right % FLAGS_width != 0) {
      if (g_marking[get_index(neighbor_right)] == 0) {
        g_marking[get_index(neighbor_right)] = distance + 1;
        if (!ds->put(neighbor_right)) {
          // We should always be able to insert an item.
          fprintf(stderr, "%s: error: put operation failed.\n", __func__);
          abort();
        }
      }
    }

    if (neighbor_below < FLAGS_width * FLAGS_height) {
      if (g_marking[get_index(neighbor_below)] == 0) {
        g_marking[get_index(neighbor_below)] = distance + 1;
        if (!ds->put(neighbor_below)) {
          // We should always be able to insert an item.
          fprintf(stderr, "%s: error: put operation failed.\n", __func__);
          abort();
        }
      }
    }

    if (neighbor_above > 0) {
      if (g_marking[get_index(neighbor_above)] == 0) {
        g_marking[get_index(neighbor_above)] = distance + 1;
        if (!ds->put(neighbor_above)) {
          // We should always be able to insert an item.
          fprintf(stderr, "%s: error: put operation failed.\n", __func__);
          abort();
        }
      }
    }
  }
}
