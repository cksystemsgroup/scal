// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/threadlocals.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>

#include "util/platform.h"
#include "util/random.h"

namespace scal {

uint64_t ThreadContext::global_thread_id_cnt = 0;
pthread_key_t ThreadContext::threadcontext_key;
ThreadContext *ThreadContext::contexts[kMaxThreads];

ThreadContext& ThreadContext::get() {
  if (pthread_getspecific(threadcontext_key) == NULL) {
    assign_context();
  }
  ThreadContext *context = static_cast<ThreadContext*>(
      pthread_getspecific(threadcontext_key));
  return *context;
}

void ThreadContext::assign_context() {
  uint64_t thread_id = __sync_fetch_and_add(&global_thread_id_cnt, 1);
  if (pthread_setspecific(threadcontext_key, contexts[thread_id])) {
    fprintf(stderr, "%s: pthread_setspecific failed\n", __func__);
    exit(EXIT_FAILURE);
  }
}

void ThreadContext::prepare(uint64_t num_threads) {
  pthread_key_create(&threadcontext_key, NULL);
  size_t size = (sizeof(ThreadContext) / scal::kPageSize + 1) * scal::kPageSize;
  void *mem;
  for (uint64_t i = 0; i < num_threads; i++) {
    if (posix_memalign(&mem, scal::kPageSize, size)) {
      fprintf(stderr, "%s: posix_memalign failed\n", __func__);
      exit(EXIT_FAILURE);
    }
    ThreadContext *context = new(mem) ThreadContext();
    context->thread_id_ = i;
    context->new_random_seed();
    contexts[i] = context;
  }
};

}  // namespace scal
