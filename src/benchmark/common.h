// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_BENCHMARK_COMMON_H_
#define SCAL_BENCHMARK_COMMON_H_

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace scal {

class Benchmark {
 public:
  Benchmark(uint64_t num_threads,
            uint64_t thread_prealloc_size,
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

  inline uint64_t num_threads() {
    return num_threads_;
  }

 private:
  static void* pthread_start_helper(void *thiz) {
    reinterpret_cast<Benchmark*>(thiz)->startup_thread();
    return NULL;
  }

  static void* pthread_dummy_func(void *nothing) {
    return NULL;
  }

  pthread_barrier_t start_barrier_;
  pthread_t *threads_;
  uint64_t num_threads_;
  uint64_t global_start_time_;
  uint64_t global_end_time_;
  uint64_t thread_prealloc_size_;

  void startup_thread(void);
  void set_core_affinity();
  void setup_pthread_attr(pthread_attr_t *attr);
  bool can_modify_sched();

  inline void handle_pthread_error(int errcode, const char* msg) {
    errno = errcode;
    perror(msg);
    exit(EXIT_FAILURE);
  }
};

}  // namespace scal

#endif  // SCAL_BENCHMARK_COMMON_H_
