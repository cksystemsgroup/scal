// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ALLOCATION_H_
#define SCAL_UTIL_ALLOCATION_H_

#include <pthread.h>
#include <stdint.h>

namespace scal {

void* MallocAligned(size_t size, size_t alignment);

size_t HumanSizeToPages(const char* hsize, size_t len);

class ThreadLocalAllocator {
 public:
  static ThreadLocalAllocator& Get();

  ThreadLocalAllocator()
      : prealloc_size_(0),
        start_(0),
        end_(0),
        current_(0) {
  }

  void Init(size_t prealloc_size, bool touch_memory);

  void* Calloc(size_t num, size_t size);
  void* CallocAligned(size_t num, size_t size, size_t alignment);

  void* Malloc(size_t size);
  void* MallocAligned(size_t size, size_t alignment);

  bool TryFreeLast();

 private:
  static void CreateTlaKey();

  void ResetBuffer();

  static pthread_key_t tla_key;

  size_t prealloc_size_;  // In bytes.
  intptr_t start_;
  intptr_t end_;
  intptr_t current_;
  // Record of the last allocated block to allow free-ing it.
  size_t last_size_;
};


template<int ALIGN>
class ThreadLocalMemory {
 public:
  static void* operator new(size_t size);
  static void operator delete(void* ptr);
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
}

}  // namespace scal

#endif  // SCAL_UTIL_ALLOCATION_H_
