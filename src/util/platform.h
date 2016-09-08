// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_PLATFORM_H_
#define SCAL_UTIL_PLATFORM_H_

#include <inttypes.h>
#include <unistd.h>

#ifndef _always_line
#ifdef DEBUG
#define _always_inline inline
#else
#define _always_inline inline __attribute__((always_inline))
#endif  // DEBUG
#endif  // _always_inline

namespace scal {

const uint64_t  kPageSize = 4096;
const uint64_t  kCachelineSize = 64;
const uint64_t  kCachePrefetch = 128;
const size_t    kWordSize = sizeof(void*);

// Data structures may rely on this constant to statically limit any buffers.
const uint64_t  kMaxThreads = 128;


// For sake of simplicity: Assume posix with optional _SC_NPROCESSORS_ONLN
// variable.
_always_inline long number_of_cores() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}


_always_inline uint64_t Rdtsc() {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return  ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

}  // namespace scal

#endif  // SCAL_UTIL_PLATFORM_H_
