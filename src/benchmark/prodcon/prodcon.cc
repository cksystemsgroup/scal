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
DEFINE_uint64(producers, 1, "number of producers");
DEFINE_uint64(consumers, 1, "number of consumers");
DEFINE_uint64(operations, 1000, "number of operations per producer");
DEFINE_uint64(c, 5000, "computational workload");
DEFINE_bool(print_summary, true, "print execution summary");
DEFINE_bool(log_operations, false, "log invocation/response/linearization "
                                   "of all operations");
DEFINE_bool(barrier, false, "uses a barrier between the enqueues and "
    "dequeues such that first all elements are enqueued, then all elements"
    "are dequeued");

using scal::Benchmark;

class ProdConBench : public Benchmark {
 public:
  ProdConBench(uint64_t num_threads,
               uint64_t thread_prealloc_size,
               void *data)
                   : Benchmark(num_threads,
                               thread_prealloc_size,
                               data) {
    if (pthread_barrier_init(&prod_con_barrier_, NULL, num_threads)) {
      fprintf(stderr, "%s: error: Unable to init start barrier.\n", __func__);
      abort();
    }
  }
 protected:
  void bench_func(void);

 private:
  void producer(void);
  void consumer(void);

  pthread_barrier_t prod_con_barrier_;
};

uint64_t g_num_threads;

int main(int argc, const char **argv) {
  std::string usage("Producer/consumer micro benchmark.");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  uint64_t tlsize = scal::human_size_to_pages(FLAGS_prealloc_size.c_str(),
                                              FLAGS_prealloc_size.size());

  // Init the main program as executing thread (may use rnd generator or tl
  // allocs).
  if (FLAGS_barrier) {
    if (FLAGS_producers > FLAGS_consumers) {
      g_num_threads = FLAGS_producers;
    } else {
      g_num_threads = FLAGS_consumers;
    }
  } else {
    g_num_threads = FLAGS_producers + FLAGS_consumers;
  }
  scal::tlalloc_init(tlsize, true /* touch pages */);
  //threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

  if (FLAGS_log_operations) {
    scal::StdOperationLogger::prepare(g_num_threads + 1,
                                      2 * g_num_threads * FLAGS_operations);
  }

  void *ds = ds_new();

  ProdConBench *benchmark = new ProdConBench(
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
    uint32_t n = snprintf(buffer, sizeof(buffer), 
        "threads: %" PRIu64 " ;producers: %" PRIu64 " consumers: %" PRIu64 " ;runtime: %" PRIu64 " ;operations: %" PRIu64 " ;c: %" PRIu64 " ;aggr: %" PRIu64 " ;ds_stats: ",
        g_num_threads - 1,
        FLAGS_producers,
        FLAGS_consumers,
        exec_time,
        FLAGS_operations,
        FLAGS_c,
        // TODO Change the calculation.
        (uint64_t)((FLAGS_operations * FLAGS_producers * 2) / (static_cast<double>(exec_time) / 1000)));
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

void ProdConBench::producer(void) {
  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  uint64_t item;
  // Do not use 0 as value, since there may be datastructures that do not
  // support it.
  for (uint64_t i = 1; i <= FLAGS_operations; i++) {
    item = thread_id * FLAGS_operations + i;
    scal::StdOperationLogger::get().invoke(scal::LogType::kEnqueue);
    if (!ds->put(item)) {
      // We should always be able to insert an item.
      fprintf(stderr, "%s: error: put operation failed.\n", __func__);
      abort();
    }
    scal::StdOperationLogger::get().response(true, item);
    calculate_pi(FLAGS_c);
  }
}

void ProdConBench::consumer(void) {
  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  // Calculate the items each consumer has to collect.
  uint64_t operations = 
    (FLAGS_producers * FLAGS_operations) / FLAGS_consumers;

  uint64_t rest = (FLAGS_producers * FLAGS_operations) % FLAGS_consumers;
  // We assume that thread ids are increasing, starting from 1.
  if (rest >= thread_id) {
    operations++;
  }
  uint64_t j = 0;
  uint64_t ret;
  bool ok;
  while (j < operations) {
    scal::StdOperationLogger::get().invoke(scal::LogType::kDequeue);
    ok = ds->get(&ret);
    scal::StdOperationLogger::get().response(ok, ret);
    calculate_pi(FLAGS_c);
    if (!ok) {
      break;
      continue;
    }
    j++;
  }
}

void ProdConBench::bench_func(void) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  if (FLAGS_barrier) {
    if (thread_id <= FLAGS_producers) {
      producer();
    }

    global_start_time_ = 0;
    int rc = pthread_barrier_wait(&prod_con_barrier_);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
      fprintf(stderr, "%s: pthread_barrier_wait failed.\n", __func__);
      abort();
    }

    if (thread_id <= FLAGS_consumers) {
      if (global_start_time_ == 0) {
        __sync_bool_compare_and_swap(&global_start_time_, 0, get_utime());
      }
      consumer();
    }

  }else {

// The lower thread indices are assigned to the producer threads.
// As the threads with lower indices start slightly earlier, the producer
// threads already fill the queue before the consumers start. Assigning 
// the lower thread indices to the consumer threads leads to an increased
// number of null-return dequeues. We do not assign the thread id's in an
// alternating fashion because because thread-id based load balancers 
// significantly benefit from such a thread-id assignment.
    if (thread_id <= FLAGS_producers) {
      producer();
    } else {
      consumer();
    }
  }
}
