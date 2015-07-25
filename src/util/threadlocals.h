// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_THREADLOCALS_H_
#define SCAL_UTIL_THREADLOCALS_H_

#include <pthread.h>
#include <stdint.h>

#include "src/util/random.h"

namespace scal {

class ThreadContext {
 public:
  static ThreadContext& get();
  static void prepare(uint64_t num_threads);
  static void assign_context();

  static constexpr uint64_t get_max_threads() {
    return kMaxThreads;
  }

  inline uint64_t thread_id() {
    return thread_id_;
  }

  inline void new_random_seed() {
    random_seed_ = static_cast<uint32_t>(
        scal::hwrand()) + (thread_id() + 1 * 100);
  }

  inline void set_random_seed(uint32_t seed) {
    random_seed_ = seed;
  }

  inline int32_t random_seed() {
    return random_seed_;
  }

  inline void set_data(void *data) {
    data_ = data;
  }

  inline void* get_data() {
    return data_;
  }

 private:
  static constexpr uint64_t kMaxThreads = 1024;
  static uint64_t global_thread_id_cnt;
  static ThreadContext *contexts[kMaxThreads];
  static pthread_key_t threadcontext_key;

  ThreadContext() {}
  ThreadContext(ThreadContext const &cpy);
  void operator=(ThreadContext const &rhs);

  uint64_t  thread_id_;
  int32_t   random_seed_; 
  void*     data_;
};

}  // namespace scal

#endif  // SCAL_UTIL_THREADLOCALS_H_
