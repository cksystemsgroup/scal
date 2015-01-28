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

#include "benchmark/common.h"
#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/pool.h"
#include "util/malloc.h"
#include "util/operation_logger.h"
#include "util/random.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/workloads.h"

DEFINE_string(prealloc_size, "1g", "tread local space that is initialized");
DEFINE_uint64(threads, 1, "number of threads");
DEFINE_uint64(elements, 1000, "number of elements enqueued per thread");
DEFINE_uint64(prefill, 0, "number of elements enqueued per thread before "
                          "the first dequeue");
DEFINE_uint64(c, 5000, "computational workload");
DEFINE_bool(print_summary, true, "print execution summary");
DEFINE_bool(log_operations, false, "log invocation/response/linearization "
                                   "of all operations");
DEFINE_bool(allow_empty_returns, false, "does not stop the execution at an "
                                   "empty-dequeue");
using scal::Benchmark;

class SeqAltBench : public Benchmark {
 public:
  SeqAltBench(uint64_t num_threads,
               uint64_t thread_prealloc_size,
               void *data)
                   : Benchmark(num_threads,
                               thread_prealloc_size,
                               data) {
  }
 protected:
  void bench_func(void);

 private:
};

uint64_t g_num_threads;

int main(int argc, const char **argv) {
  std::string usage("SeqAlt micro benchmark.");
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

  if (FLAGS_log_operations) {
    scal::StdOperationLogger::prepare(g_num_threads + 1,
                                      2 * FLAGS_elements);
  }

  void *ds = ds_new();

  SeqAltBench *benchmark = new SeqAltBench(
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
    uint32_t n = snprintf(buffer, sizeof(buffer), "threads: %" PRIu64 " ;runtime: %" PRIu64 " ;operations: %" PRIu64 " ;c: %" PRIu64 " ;aggr: %" PRIu64 " ;ds_stats: ",
        FLAGS_threads,
        exec_time,
        FLAGS_elements,
        FLAGS_c,
        (uint64_t)((FLAGS_elements * 2 * FLAGS_threads) / (static_cast<double>(exec_time) / 1000)));
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

void SeqAltBench::bench_func(void) {

  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  uint64_t item;
  // Do not use 0 as value, since there may be datastructures that do not
  // support it.
  for (uint64_t i = 1; i <= FLAGS_elements + FLAGS_prefill - 1; i++) {
    if (i <= FLAGS_elements) {
      item = thread_id * FLAGS_elements + i;
      scal::StdOperationLogger::get().invoke(scal::LogType::kEnqueue);
      if (!ds->put(item)) {
        // We should always be able to insert an item.
        fprintf(stderr, "%s: error: put operation failed.\n", __func__);
        abort();
      }
      scal::StdOperationLogger::get().response(true, item);
      calculate_pi(FLAGS_c);
    }
    
    if (i >= FLAGS_prefill) {
      scal::StdOperationLogger::get().invoke(scal::LogType::kDequeue);
      if (!ds->get(&item)) {
        if (!FLAGS_allow_empty_returns) {
          // We should always be able to get an item.
          fprintf(stderr, "%s: error: get operation failed.\n", __func__);
          abort();
        }
      }
      scal::StdOperationLogger::get().response(true, item);
      calculate_pi(FLAGS_c);
    }
  }
}
