// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/threadlocals.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

namespace {

const uint64_t kPageSize = 4096;
const uint64_t kMaxThreads = 1024;

pthread_key_t threadlocals_key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;
pthread_once_t init_once = PTHREAD_ONCE_INIT;
uint64_t id_cnt = 0;
threadlocals_t *threadlocals[kMaxThreads];

void make_key(void) {
  pthread_key_create(&threadlocals_key, NULL);
}

void init_thread(void) {
}

}  // namespace

void threadlocals_init(void) {
  pthread_once(&key_once, make_key);
  if (pthread_getspecific(threadlocals_key)) {
    return;
  }
  threadlocals_t *t;
  uint64_t size = (sizeof(threadlocals_t) / kPageSize + 1) * kPageSize;
  if (posix_memalign(reinterpret_cast<void**>(&t),
                     kPageSize,
                     size)) {
    fprintf(stderr, "%s: posix_memalign failed.\n", __func__);
    abort();
  }
  t->thread_id = __sync_fetch_and_add(&id_cnt, 1);
  t->data = NULL;
  if (pthread_setspecific(threadlocals_key, t)) {
    fprintf(stderr, "%s: pthread_setspecific failed.\n", __func__);
    abort();
  }
  threadlocals[t->thread_id] = t;

  scal::srand(scal::hwrand() + (t->thread_id + 1) * 100);
}

threadlocals_t* threadlocals_get(void) {
  return static_cast<threadlocals_t*>(pthread_getspecific(threadlocals_key));
}

threadlocals_t* threadlocals_get(uint64_t thread_id) {
  return threadlocals[thread_id];
}

uint64_t threadlocals_num_threads(void) {
  return id_cnt;
}

namespace scal {

uint64_t ThreadContext::global_thread_id_cnt = 0;
pthread_key_t ThreadContext::threadcontext_key;
ThreadContext *ThreadContext::contexts[kMaxThreads];

ThreadContext& ThreadContext::get() {
  ThreadContext *context = static_cast<ThreadContext*>(
      pthread_getspecific(threadcontext_key));
  return *context;
}

bool ThreadContext::try_assign_context() {
  if (pthread_getspecific(threadcontext_key)) {
    return false;  // already assigned
  }
  uint64_t thread_id = __sync_fetch_and_add(&global_thread_id_cnt, 1);
  if (pthread_setspecific(threadcontext_key, contexts[thread_id])) {
    fprintf(stderr, "%s: pthread_setspecific failed\n", __func__);
    exit(EXIT_FAILURE);
  }
  return true;
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
