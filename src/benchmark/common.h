// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_BENCHMARK_COMMON_H_
#define SCAL_BENCHMARK_COMMON_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace scal {

class Benchmark {
 public:
  Benchmark(uint64_t num_threads,
            uint64_t thread_prealloc_size,
            uint64_t histogram_size,
            void *data);
  void run(void);

  inline uint64_t execution_time(void) {
    return global_end_time_ - global_start_time_;
  }

 protected:
  virtual ~Benchmark() {}

  void *data_;

  virtual void bench_func(void) = 0;
  uint64_t thread_id(void);

 private:
  static void *pthread_start_helper(void *thiz) {
    reinterpret_cast<Benchmark*>(thiz)->startup_thread();
    return NULL;
  }

  pthread_barrier_t start_barrier_;
  pthread_t *threads_;
  uint64_t num_threads_;
  uint64_t global_start_time_;
  uint64_t global_end_time_;
  uint64_t thread_prealloc_size_;

  void startup_thread(void);
};

}  // namespace scal

#endif  // SCAL_BENCHMARK_COMMON_H_
