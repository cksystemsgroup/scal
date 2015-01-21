// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// All functions should be replaced by corresponding concepts from
// util/allocation.h. For backwards compatibility we still ship this file. Note
// that we already use the new stuff in the background.

#ifndef SCAL_UTIL_MALLOC_COMPAT_H_
#define SCAL_UTIL_MALLOC_COMPAT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <new>  // placement new()

#include "util/allocation.h"

namespace scal {

inline uint64_t human_size_to_pages(const char *hsize, size_t len) {
  return scal::HumanSizeToPages(hsize, len);
}


inline void* malloc_aligned(size_t size, size_t alignment) {
  return scal::MallocAligned(size, alignment);
}


inline void* calloc_aligned(size_t size, size_t alignment) {
  void* obj = malloc_aligned(size, alignment);
  memset(obj, 1, size);
  return obj;
}


// Deprecated: Use scal::get<T, alignment>().
template<typename T>
T* get_aligned(uint64_t alignment) {
  void *mem;
  if (alignment == 0) {  // no alignment
    mem = calloc(1, sizeof(T));
  } else {
    mem = malloc_aligned(sizeof(T), alignment);
    memset(mem, 0, sizeof(T));
  }
  T *obj = new(mem) T();
  return obj;
}


// Deprecated: Use scal::get<T, alignment>().
template<typename T>
T* get(uint64_t alignment) {
  return get_aligned<T>(alignment);
}


template<typename T, int alignment>
T* get() {
  void* mem;
  if (alignment == 0) {
    mem = malloc(sizeof(T));
  } else {
    mem = malloc_aligned(sizeof(T), alignment);
  }
  T* obj = new(mem) T();
  return obj;
}


inline void tlalloc_init(size_t prealloc_size, bool touch_pages) {
  scal::ThreadLocalAllocator::Get().Init(prealloc_size, touch_pages);
}


inline void* tlmalloc(size_t size) {
  return scal::ThreadLocalAllocator::Get().Malloc(size);
}


inline void* tlcalloc(size_t num, size_t size) {
  return scal::ThreadLocalAllocator::Get().Calloc(num, size);
}


inline void* tlmalloc_aligned(size_t size, size_t alignment) {
  return scal::ThreadLocalAllocator::Get().MallocAligned(size, alignment);
}


inline void* tlcalloc_aligned(size_t num, size_t size, size_t alignment) {
  return scal::ThreadLocalAllocator::Get().CallocAligned(num, size, alignment);
}


inline void tl_free_last(void) {
  scal::ThreadLocalAllocator::Get().TryFreeLast();
}


//
// Deprecated: Use scal::tlget<T, alignment>().
template<typename T>
T* tlget_aligned(uint64_t alignment) {
  void *mem;
  if (alignment == 0) {  // no alignment
    mem = tlcalloc(1, sizeof(T));
  } else {
    mem = tlcalloc_aligned(1, sizeof(T), alignment);
  }
  T *obj = new(mem) T();
  return obj;
}


// Deprecated: Use scal::tlget<T, alignment>().
template<typename T>
T* tlget(uint64_t alignment) {
  return tlget_aligned<T>(alignment);
}


template<typename T, int alignment>
T* tlget() {
  void* mem;
  if (alignment == 0) {
    mem = tlmalloc(sizeof(T));
  } else {
    mem = tlmalloc_aligned(sizeof(T), alignment);
  }
  T* obj = new(mem) T();
  return obj;
}

}  // namespace scal

#endif  // SCAL_UTIL_MALLOC_COMPAT_H_
