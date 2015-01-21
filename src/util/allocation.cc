// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "allocation.h"

#include <assert.h>
#include <gflags/gflags.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <new>

DEFINE_bool(reuse_memory_new, true, "reuse memory, no matter what");

namespace {

const size_t kPageSize = 4096;
const size_t kWordSize = sizeof(void*);

pthread_once_t tla_key_once = PTHREAD_ONCE_INIT;


inline size_t RoundSize(uint64_t size, uint64_t round_to) {
  if ((round_to == 0) || (size % round_to) == 0) {
    return size;
  } 
  return ((size / round_to) + 1) * round_to;
}

}  // namespace


namespace scal {

pthread_key_t ThreadLocalAllocator::tla_key;


size_t HumanSizeToPages(const char* hsize, size_t len) {
  if (hsize[len] != 0) {
    fprintf(stderr, "%s: epected valid c string\n", __func__);
    abort();
  } 
  size_t multiplier;
  switch (hsize[len-1]) {
  case 'k':
  case 'K':
    multiplier = 1024;
    break;
  case 'm':
  case 'M':
    multiplier = 1024 * 1024;
    break;
  case 'g':
  case 'G':
    multiplier = 1024 * 1024 * 1024;
    break;
  default:
    multiplier = 1;
  }
  const size_t val = strtol(hsize, NULL, 10);
  if (val == 0) {
    fprintf(stderr, "%s: strtol() returned 0; do not try to set the "
                    "prealloc_size to 0\n", __func__);
    abort();
  }
  const size_t bytes = val * multiplier;
  return ((bytes % kPageSize) == 0) ? bytes/kPageSize : bytes/kPageSize + 1;
}


void* MallocAligned(size_t size, size_t alignment) {
  void* mem;
  if (posix_memalign(reinterpret_cast<void**>(&mem),
                     alignment, size)) {
    perror("posix_memalign");
    abort();
  }
  return mem;
}


void ThreadLocalAllocator::CreateTlaKey() {
  pthread_key_create(&tla_key, NULL);
}


ThreadLocalAllocator& ThreadLocalAllocator::Get() {
  pthread_once(&tla_key_once, ThreadLocalAllocator::CreateTlaKey);
  ThreadLocalAllocator* tla = static_cast<ThreadLocalAllocator*>(
      pthread_getspecific(tla_key));
  if (tla == NULL) {
    const size_t tla_size = RoundSize(sizeof(*tla), kPageSize);
    void* mem = scal::MallocAligned(tla_size, kPageSize);
    tla = new(mem) ThreadLocalAllocator();
    if (pthread_setspecific(tla_key, tla)) {
      perror("pthread_setspecific");
      abort();
    }
  }
  return *tla;
}


void ThreadLocalAllocator::ResetBuffer() {
  current_ = start_;
  end_ = start_ + prealloc_size_;
  last_size_ = 0;
}


void ThreadLocalAllocator::Init(size_t prealloc_pages, bool touch_memory) {
  prealloc_size_ = kPageSize * prealloc_pages;
  start_ = reinterpret_cast<uintptr_t>(
      scal::MallocAligned(prealloc_size_, kPageSize));
  ResetBuffer();
  if (touch_memory) {
    for (size_t i = 0; i < (prealloc_size_ / sizeof(intptr_t)); i++) {
      reinterpret_cast<intptr_t*>(start_)[i] = 1;
      reinterpret_cast<intptr_t*>(start_)[i] = 0;
    }
  }
}


void* ThreadLocalAllocator::Malloc(size_t size) {
  size = RoundSize(size, 2 * kWordSize);
  if (size > prealloc_size_) {
    fprintf(stderr, "unable to allocate %lu bytes through "
                    "thread-local allocation", size);
    abort();
  }
  last_size_ = size;
  if ((current_ + static_cast<intptr_t>(size)) >= end_) {
    if (FLAGS_reuse_memory_new) {
      ResetBuffer();
    } else {
      fprintf(stderr, "warning: allocating new buffer\n");
      start_ = reinterpret_cast<uintptr_t>(
          scal::MallocAligned(prealloc_size_, kPageSize));
      ResetBuffer();
    }
  }
  void* object = reinterpret_cast<void*>(current_);
  current_ += size;
  return object;
}


void* ThreadLocalAllocator::Calloc(size_t size, size_t num) {
  const size_t alloc_size = RoundSize(size * num, 2 * kWordSize);
  void* object = Malloc(alloc_size);
  memset(object, 0, alloc_size);
  return object;
}


void* ThreadLocalAllocator::MallocAligned(size_t size, size_t alignment) {
  // TODO: Check alignment.
  size = RoundSize(size, 2 * kWordSize);
  const size_t alloc_size = size + alignment;
  intptr_t start = reinterpret_cast<intptr_t>(Malloc(alloc_size));
  start += alignment - (start % alignment);
  assert((start % alignment) == 0);
  return reinterpret_cast<void*>(start);   
}


void* ThreadLocalAllocator::CallocAligned(size_t num, size_t size, size_t alignment) {
  const size_t alloc_size = RoundSize(num * size, 2 * kWordSize);
  void* object = MallocAligned(alloc_size, alignment);
  memset(object, 0, alloc_size);
  return object;
}


bool ThreadLocalAllocator::TryFreeLast() {
  if (last_size_ > 0) {
    current_ -= last_size_;
    last_size_ = 0;
    return true;
  }
  return false;
}

}  // namespace scal
