// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/malloc.h"

#include <errno.h>
#include <gflags/gflags.h>
#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DEFINE_bool(reuse_memory, true, "reuse memory, no matter what");
DEFINE_bool(disable_tl_allocator, false, "all thread local calls are mapped"
                                         " to malloc");

namespace {

struct MemBuffer {
  void* memory;
  size_t mem_size;
  void* pointer;
  void* start;
  uint64_t wrap_around_cnt;
  uint64_t last_size;
};

const void* kWord;
const uint64_t kPageSize = 4096;
const uint64_t kTLABSize = kPageSize;  // carefull!!
pthread_once_t key_once = PTHREAD_ONCE_INIT;
pthread_key_t talloc_key;

void make_pthread_key(void) {
  pthread_key_create(&talloc_key, NULL);
}

inline size_t align_size(uint64_t size, uint64_t alignment) {
  if ((alignment == 0) || (size % alignment == 0)) {
    return size;
  } else {
    return (size / alignment + 1) * alignment;
  }
}

MemBuffer* tl_buffer_get(void) {
  MemBuffer *buffer = static_cast<MemBuffer*>(pthread_getspecific(talloc_key));
  if (buffer == NULL) {
    if (posix_memalign(reinterpret_cast<void**>(&buffer),
                       kPageSize,
                       sizeof(MemBuffer))) {
      perror("posix_memalign");
      abort();
    }
    if (buffer == NULL) {
      perror("malloc");
      abort();
    }
    buffer->memory = NULL;
    buffer->pointer = NULL;
    if (pthread_setspecific(talloc_key, buffer)) {
      perror("pthread_setspecific");
      abort();
    }
    buffer->last_size = 0;
  }
  return buffer;
}

}  // namespace

namespace scal {

uint64_t human_size_to_pages(const char *hsize, size_t len) {
  if (hsize[len] != 0) {
    fprintf(stderr, "%s: epected valid c string\n", __func__);
    abort();
  } 
  uint64_t multiplier;
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
  char *newstr = static_cast<char*>(calloc(len, sizeof(*newstr)));
  strncpy(newstr, hsize, len - 1);
  uint64_t val = strtol(newstr, NULL, 10);
  if (val == 0) {
    fprintf(stderr, "%s: strtol() returned 0; do not try to set the "
                    "prealloc_size to 0\n", __func__);
    abort();
  }
  uint64_t bytes = (uint64_t)val * multiplier;
  if ((bytes % 4096) == 0) {
    return bytes / 4096;
  } 
  return (bytes / 4096) + 1;
}

void* malloc_aligned(size_t size, size_t alignment) {
  void *mem;
  int err;
  err = posix_memalign(&mem, alignment, size);
  if (err == EINVAL) {
    fprintf(stderr, "%s: posix_memalign failed: The alignment argument was "
        "not a power of two, or was not a multiple of "
        "sizeof(void *).\n", __func__);
    abort();
  } else if (err == ENOMEM) {
    fprintf(stderr, "%s: posix_memalign failed: Insufficient memory.\n",
        __func__);
    abort();
  }
  return mem;
}

void* calloc_aligned(size_t num, size_t size, size_t alignment) {
  void *mem = malloc_aligned(num * size, alignment);
  memset(mem, 0, align_size(num * size, alignment));
  return mem;
}

void tlalloc_init(uint64_t num_tlabs, bool touch_pages) {
  if (FLAGS_disable_tl_allocator) {
    return;
  }
  pthread_once(&key_once, make_pthread_key);
  MemBuffer *buffer = tl_buffer_get();
  buffer->mem_size = kTLABSize * num_tlabs;
  buffer->memory = malloc_aligned(buffer->mem_size, kPageSize);
  buffer->start = buffer->memory;
  buffer->wrap_around_cnt = 0;
  if (touch_pages) {
    for (uint64_t i = 0; i < (buffer->mem_size / sizeof(kWord)); i++) {
      reinterpret_cast<uint64_t*>(buffer->memory)[i] = 1;
      reinterpret_cast<uint64_t*>(buffer->memory)[i] = 0;
    }
  }
  buffer->pointer = buffer->memory;
}

void* tlmalloc(size_t size) {
  if (FLAGS_disable_tl_allocator) {
    return malloc(size);
  }
  pthread_once(&key_once, make_pthread_key);
  size = align_size(size, 2 * sizeof(kWord));
  if (size > kPageSize) {
    return malloc(size);
  }
  MemBuffer *buffer = tl_buffer_get();
  uint64_t buffer_ptr = reinterpret_cast<uint64_t>(buffer->pointer);
  uint64_t buffer_mem = reinterpret_cast<uint64_t>(buffer->memory);
  if ((buffer->memory == NULL)
      || (buffer_ptr + size > buffer_mem + buffer->mem_size)) {
    if (FLAGS_reuse_memory) {
      buffer->memory = buffer->start;
      buffer->pointer = buffer->memory;
    } else {
      fprintf(stderr, "warning: tlalloc is dynamically allocating memory\n");
      buffer->mem_size = kTLABSize;
      if (posix_memalign(&buffer->memory, kPageSize, buffer->mem_size)) {
        perror("posix_memalign");
        abort();
      }
      buffer->pointer = buffer->memory;
    }
  }

  void *object = buffer->pointer;
  buffer_ptr = reinterpret_cast<uint64_t>(buffer->pointer);
  buffer->pointer = reinterpret_cast<void*>(buffer_ptr + size);
  buffer->last_size = size;
  return object;
}

void* tlcalloc(size_t num, size_t size) {
  size *= num;
  void *object = tlmalloc(size);
  size = align_size(size, 2 * sizeof(kWord));
  memset(object, 0, size);
  return object;
}

void* tlmalloc_aligned(size_t size, size_t alignment) {
  if (FLAGS_disable_tl_allocator) {
    return malloc_aligned(size, alignment);
  }
  pthread_once(&key_once, make_pthread_key);
  MemBuffer *buffer = tl_buffer_get();
  if (alignment == 0) {
    return tlmalloc(size);
  }
  if (alignment > 4096) {
    fprintf(stderr,
            "%s: warning: Alignment > page size not supported. "
            "Using page size.\n",
            __func__);
    alignment = 4096;
  }

  uint64_t old_pointer = (uint64_t)(buffer->pointer);
  uint64_t new_pointer = align_size(old_pointer, alignment);
  size = align_size(size, alignment);

  uint64_t memory_adr = (uint64_t)(buffer->memory);
  if (buffer->memory == NULL
      || ((new_pointer + size) > (memory_adr + buffer->mem_size))) {
    if (FLAGS_reuse_memory) {
      buffer->memory = buffer->start;
      buffer->pointer = buffer->memory;
      buffer->wrap_around_cnt++;
      fprintf(stderr, "warning: overflowing\n");
    } else {
      fprintf(stderr, "warning: tlalloc is dynamically allocating memory\n");
      buffer->mem_size = kTLABSize;
      if (posix_memalign(&buffer->memory, kPageSize, buffer->mem_size)) {
        perror("posix_memalign");
        abort();
      }
      buffer->pointer = buffer->memory;
    }
    old_pointer = reinterpret_cast<uint64_t>(buffer->pointer);
    new_pointer = align_size(old_pointer, alignment);
  }
  void *object = reinterpret_cast<void*>(new_pointer);
  buffer->pointer = reinterpret_cast<void*>(new_pointer + size);
  buffer->last_size = size;
  return object;
}

void* tlcalloc_aligned(size_t num, size_t size, size_t alignment) {
  size *= num;
  void* object = tlmalloc_aligned(size, alignment);
  size = align_size(size, alignment);
  memset(object, 0, size);
  return object;
}

void tl_free_last(void) {
  MemBuffer *buffer = tl_buffer_get();
  if (buffer->last_size == 0) {
    fprintf(stderr, "%s: error: last malloc already freed.\n", __func__);
    abort();
  }
  // This algorithm assumes that an object is spans continous memory.
  uint64_t old_pointer = (uint64_t)(buffer->pointer);
  buffer->pointer = reinterpret_cast<void*>(old_pointer - buffer->last_size);
  buffer->last_size = 0;
}

void tlprint_wrap_around(void) {
  pthread_once(&key_once, make_pthread_key);
  MemBuffer *buffer = tl_buffer_get();
  printf("%lu\n", buffer->wrap_around_cnt);
}

}  // namespace scal
