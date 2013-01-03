// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "benchmark/common.h"

#include "util/malloc.h"
#include "util/time.h"
#include "util/threadlocals.h"

namespace scal {

Benchmark::Benchmark(uint64_t num_threads,
                     uint64_t thread_prealloc_size,
                     uint64_t histogram_size,
                     void *data) {
  num_threads_ = num_threads;
  data_ = data;
  thread_prealloc_size_ = thread_prealloc_size;
  if (pthread_barrier_init(&start_barrier_, NULL, num_threads_)) {
    fprintf(stderr, "%s: error: Unable to init start barrier.\n", __func__);
    abort();
  }
  threads_ = static_cast<pthread_t*>(malloc(num_threads_ * sizeof(pthread_t)));
  if (!threads_) {
    perror("malloc");
    abort();
  }
}

void Benchmark::run(void) {
  int ret_code;
  global_start_time_ = 0;
  for (uint64_t i = 0; i < num_threads_; i++) {
    ret_code = pthread_create(&threads_[i],
                              NULL,
                              Benchmark::pthread_start_helper,
                              reinterpret_cast<void*>(this));
    if (ret_code) {
      fprintf(stderr, "%s: pthread_create failed.\n", __func__);
      abort();
    }
  }
  for (uint64_t i = 0; i < num_threads_; i++) {
    pthread_join(threads_[i], NULL);
  }
  global_end_time_ = get_utime();
}

void Benchmark::startup_thread(void) {
  scal::tlalloc_init(thread_prealloc_size_, true /* touch pages */);
  threadlocals_init();
  uint64_t thread_id = threadlocals_get()->thread_id;
  if (thread_id == 0) {
    fprintf(stderr, "%s: error: thread_id should be main thread. "
                    "Did you forged to init the main thread?\n", __func__);
    abort();
  }
  int rc = pthread_barrier_wait(&start_barrier_);
  if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "%s: pthread_barrier_wait failed.\n", __func__);
    abort();
  }
  if (global_start_time_ == 0) {
    __sync_bool_compare_and_swap(&global_start_time_, 0, get_utime());
  }
  bench_func();
}

uint64_t Benchmark::thread_id(void) {
  return threadlocals_get()->thread_id;
}

}  // namespace scal
