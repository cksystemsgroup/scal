// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// We use our own allocator where necessary. All functions use pthreads,
// malloc, and posix_memalign underneath.

#ifndef SCAL_UTIL_MALLOC_H_
#define SCAL_UTIL_MALLOC_H_

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

namespace scal {

uint64_t human_size_to_pages(const char *hsize, size_t len);

// convenience methods
void* malloc_aligned(size_t size, size_t alignment);
void* calloc_aligned(size_t num, size_t size, size_t alignment);

inline void posix_memalign_err(int err, const char *caller) {
  if (err == EINVAL) {
    fprintf(stderr, "%s: error: posix_memalign: The alignment argument was "
        "not a power of two, or was not a multiple of sizeof(void *).\n",
        caller);
    abort();
  } else if (err == ENOMEM) {
    fprintf(stderr, "%s: error: posix_memalign: Insufficient memory.\n",
        caller);
    abort();
  }
}

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

template<typename T>
T* get(uint64_t alignment) {
  return get_aligned<T>(alignment);
}

// Threadlocal allocator:
// Call tlalloc_init before anything else in the corresponding thread.
// Routines are named tl<func> and tl<func>_aligned, where <func> is the
// name of the function one would usually use, i.e. tlmalloc for
// threadlocal malloc.
void tlalloc_init(uint64_t num_tlabs, bool touch_pages);
void* tlmalloc(size_t size);
void* tlcalloc(size_t num, size_t size);
void* tlmalloc_aligned(size_t size, size_t alignment);
void* tlcalloc_aligned(size_t num, size_t size, size_t alignment);
void tl_free_last(void);

void tlprint_wrap_around(void);

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

template<typename T>
T* tlget(uint64_t alignment) {
  return tlget_aligned<T>(alignment);
}

}  // namespace scal

#endif  // SCAL_UTIL_MALLOC_H_
