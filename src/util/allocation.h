// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ALLOCATION_H_
#define SCAL_UTIL_ALLOCATION_H_

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus

#include <gflags/gflags.h>

#include <new>

#include "util/platform.h"

DECLARE_bool(reuse_memory);
DECLARE_bool(warn_on_overflow);

namespace scal {

extern pthread_once_t tla_key_once;

size_t HumanSizeToPages(const char* hsize, size_t len);


_always_inline size_t RoundSize(uint64_t size, uint64_t round_to) {
  if ((round_to == 0) || (size % round_to) == 0) {
    return size;
  }
  return ((size / round_to) + 1) * round_to;
}


_always_inline void* MallocAligned(size_t size, size_t alignment) {
  void* mem;
  if (posix_memalign(reinterpret_cast<void**>(&mem),
                     alignment, size)) {
    perror("posix_memalign");
    abort();
  }
  return mem;
}


_always_inline void* CallocAligned(size_t num, size_t size, size_t alignment) {
  const size_t sz = num * size;
  void* mem = MallocAligned(sz, alignment);
  memset(mem, 0, sz);
  return mem;
}


class ThreadLocalAllocator {
 public:
  static _always_inline ThreadLocalAllocator& Get();

  _always_inline ThreadLocalAllocator()
      : prealloc_size_(0),
        start_(0),
        end_(0),
        current_(0) {
  }

  _always_inline void Init(size_t prealloc_size, bool touch_memory);
  _always_inline void* Calloc(size_t num, size_t size);
  _always_inline void* CallocAligned(size_t num, size_t size, size_t alignment);
  _always_inline void* Malloc(size_t size);
  _always_inline void* MallocAligned(size_t size, size_t alignment);
  _always_inline bool TryFreeLast();

 private:
  static inline void CreateTlaKey();

  _always_inline void ResetBuffer();

  static pthread_key_t tla_key;

  size_t prealloc_size_;  // In bytes.
  intptr_t start_;
  intptr_t end_;
  intptr_t current_;
  // Record of the last allocated block to allow free-ing it.
  size_t last_size_;
};


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
  if ((current_ + static_cast<intptr_t>(size)) >= end_) {
    if (FLAGS_reuse_memory) {
      if (FLAGS_warn_on_overflow) {
        fprintf(stderr, "%p: overflowing buffer. resetting.\n", this);
      }
    } else {
      fprintf(stderr, "warning: allocating new buffer\n");
      start_ = reinterpret_cast<uintptr_t>(
          scal::MallocAligned(prealloc_size_, kPageSize));
    }
    ResetBuffer();
  }
  last_size_ = size;
  void* object = reinterpret_cast<void*>(current_);
  current_ += size;
  return object;
}


void* ThreadLocalAllocator::MallocAligned(size_t size, size_t alignment) {
  // TODO(ckgroup): Check alignment.
  size = RoundSize(size, 2 * kWordSize);
  const size_t alloc_size = size + alignment;
  intptr_t start = reinterpret_cast<intptr_t>(Malloc(alloc_size));
  start += alignment - (start % alignment);
  assert((start % alignment) == 0);
  return reinterpret_cast<void*>(start);
}


void* ThreadLocalAllocator::Calloc(size_t size, size_t num) {
  const size_t alloc_size = RoundSize(size * num, 2 * kWordSize);
  void* object = Malloc(alloc_size);
  memset(object, 0, alloc_size);
  return object;
}


void* ThreadLocalAllocator::CallocAligned(
    size_t num, size_t size, size_t alignment) {
  const size_t alloc_size = RoundSize(num * size, 2 * kWordSize);
  void* object = MallocAligned(alloc_size, alignment);
  memset(object, 0, alloc_size);
  return object;
}


void ThreadLocalAllocator::ResetBuffer() {
  current_ = start_;
  end_ = start_ + prealloc_size_;
  last_size_ = 0;
}


bool ThreadLocalAllocator::TryFreeLast() {
  if (last_size_ > 0) {
    current_ -= last_size_;
    last_size_ = 0;
    return true;
  }
  return false;
}


template<int ALIGN>
class ThreadLocalMemory {
 public:
  static _always_inline void* operator new(size_t size);
  static _always_inline void operator delete(void* ptr);
};


template<int ALIGN>
inline void* ThreadLocalMemory<ALIGN>::operator new(size_t size) {
  return ThreadLocalAllocator::Get().CallocAligned(1, size, ALIGN);
}


template<>
inline void* ThreadLocalMemory<0>::operator new(size_t size) {
  return ThreadLocalAllocator::Get().Calloc(1, size);
}


template<int ALIGN>
inline void ThreadLocalMemory<ALIGN>::operator delete(void* ptr) {
  ThreadLocalAllocator::Get().TryFreeLast();
}

}  // namespace scal

#endif  // __cplusplus

// Provide a C interface.
//
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void* tla_malloc(size_t size);
void* tla_malloc_aligned(size_t size, size_t alignment);


#ifdef __cplusplus
}
#endif  // __cpluscplus

#endif  // SCAL_UTIL_ALLOCATION_H_
