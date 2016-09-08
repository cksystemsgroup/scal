// Copyright (c) 2015, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benchmark/common.h"
#include "datastructures/ms_queue.h"
#include "util/threadlocals.h"
#include "util/scal-time.h"
#include "util/workloads.h"

DEFINE_string(prealloc_size, "10m", "tread local space that is initialized");
DEFINE_uint64(threads, 1, "number of threads");
DEFINE_uint64(operations, 10000000, "number of operations per producer");
DEFINE_uint64(c, 5000, "computational workload");
DEFINE_bool(use_rdtsc_load, true, "use rdtsc wait for computational load");


class CBench : public scal::Benchmark {
 public:
  CBench(uint64_t num_threads, uint64_t thread_prealloc_size);

 protected:
  void bench_func();
};


typedef void (*LoadFunc)(uint64_t);


uint64_t g_num_threads;
LoadFunc computation;

int main(int argc, const char **argv) {
  std::string usage("mm microbenchmark.");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  const size_t tlsize = scal::HumanSizeToPages(
      FLAGS_prealloc_size.c_str(), FLAGS_prealloc_size.size());
  g_num_threads = FLAGS_threads;
  scal::ThreadLocalAllocator::Get().Init(tlsize, true);
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

  if (FLAGS_use_rdtsc_load) {
    computation = scal::RdtscWait;
  } else {
    computation = reinterpret_cast<LoadFunc>(scal::ComputePi);
  }

  CBench benchmark(g_num_threads, tlsize);
  benchmark.run();
  printf("{ \"c\": %lu "
         ",\"operations\": %lu "
         ",\"threads\": %lu "
         ",\"time\": %f "
         "}\n",
         FLAGS_c, FLAGS_operations, FLAGS_threads, (double)benchmark.execution_time()/FLAGS_operations);
  return EXIT_SUCCESS;
}


CBench::CBench(
    uint64_t num_threads, uint64_t thread_prealloc_size)
        : Benchmark(num_threads, thread_prealloc_size, NULL) {
}


void CBench::bench_func() {
  for(uint64_t i = 0; i < FLAGS_operations; i++) {
    computation(FLAGS_c);
  }
}
