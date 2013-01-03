// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/threadlocals.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/malloc.h"

namespace {

const uint64_t kPageSize = 4096;
const uint64_t kMaxThreads = 1024;

pthread_key_t threadlocals_key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;
uint64_t id_cnt = 0;
threadlocals_t *threadlocals[kMaxThreads];

void make_key(void) {
  pthread_key_create(&threadlocals_key, NULL);
}

}  // namespace

void threadlocals_init(void) {
  pthread_once(&key_once, make_key);
  threadlocals_t *t;
  uint64_t size = (sizeof(threadlocals_t) / kPageSize + 1) * kPageSize;
  if (posix_memalign(reinterpret_cast<void**>(&t),
                     kPageSize,
                     size)) {
    fprintf(stderr, "%s: posix_memalign failed.\n", __func__);
    abort();
  }
  t->thread_id = __sync_fetch_and_add(&id_cnt, 1);
  t->random_seed = (t->thread_id + 1) * 100;
  t->data = NULL;
  if (pthread_setspecific(threadlocals_key, t)) {
    fprintf(stderr, "%s: pthread_setspecific failed.\n", __func__);
    abort();
  }
  threadlocals[t->thread_id] = t;
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
