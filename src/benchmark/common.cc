// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "benchmark/common.h"

#include <gflags/gflags.h>
#include <pthread.h>
#include <sched.h>

#include "util/allocation.h"
#include "util/malloc-compat.h"
#include "util/platform.h"
#include "util/time.h"
#include "util/threadlocals.h"

DEFINE_bool(set_rt_priority, true,
            "try to set the program to RT priority (needs root)");

namespace scal {

Benchmark::Benchmark(uint64_t num_threads,
                     uint64_t thread_prealloc_size,
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
  int s;
  pthread_attr_t attr;
  pthread_attr_t *pattr;

  if (FLAGS_set_rt_priority && can_modify_sched()) {
    setup_pthread_attr(&attr);
    pattr = &attr;
  } else {
    pattr = NULL;
  }

  global_start_time_ = 0;
  for (uint64_t i = 0; i < num_threads_; i++) {
    s = pthread_create(&threads_[i],
                       pattr,
                       Benchmark::pthread_start_helper,
                       reinterpret_cast<void*>(this));
    if (s != 0) {
      handle_pthread_error(s, "pthread_create");
    }
  }
  for (uint64_t i = 0; i < num_threads_; i++) {
    pthread_join(threads_[i], NULL);
  }
  global_end_time_ = get_utime();
}

void Benchmark::setup_pthread_attr(pthread_attr_t *attr) {
  int s;
  struct sched_param param;
  s = pthread_attr_init(attr);
  if (s != 0) {
    handle_pthread_error(s, "pthread_attr_init");
  }
  s = pthread_attr_setschedpolicy(attr, SCHED_RR);
  if (s != 0) {
    handle_pthread_error(s, "pthread_attr_setschedpolicy");
  }
  param.sched_priority = 99;
  s = pthread_attr_setschedparam(attr, &param);
  if (s != 0) {
    handle_pthread_error(s, "pthread_attr_setschedparam");
  }
  s = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  if (s != 0) {
    handle_pthread_error(s, "pthread_attr_setinheritsched");
  }
}

bool Benchmark::can_modify_sched() {
  int s;
  pthread_attr_t attr;
  pthread_t thread;
  setup_pthread_attr(&attr);
  s = pthread_create(&thread,
                     &attr,
                     Benchmark::pthread_dummy_func,
                     NULL);
  if (s != 0) {
    if (s == EPERM) {
      fprintf(stderr, "warning: unable to modify scheduler policy and "
                      "priority for worker threads. consider running "
                      "the benchmark with sufficient priviliges.\n");
      return false;
    } else {
      handle_pthread_error(s, "pthread_create");
    }
  }
  s = pthread_join(thread, NULL);
  if (s != 0) {
    handle_pthread_error(s, "pthread_join");
  }
  return true;
}

void Benchmark::set_core_affinity() {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  uint64_t cores = (uint64_t)scal::number_of_cores();
  if (num_threads_ > cores) {
    return;
  }
  uint64_t id;
  if (cores == num_threads_) {
    id = thread_id - 1;
  } else {
    double mult = (static_cast<double>(cores)) / num_threads_;
    id = (uint64_t)(thread_id * mult) - (uint64_t)mult;
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET((uint64_t)id, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
    fprintf(stderr, "warning: could not set thread cpu affinity\n");
  }
}

void Benchmark::startup_thread() {
  scal::ThreadLocalAllocator::Get().Init(thread_prealloc_size_, true);
  //set_core_affinity();
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
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
  return scal::ThreadContext::get().thread_id();
}

}  // namespace scal
