// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_TIME_H_
#define SCAL_UTIL_TIME_H_

#include <stdint.h>
#include <sys/time.h>

inline uint64_t get_utime(void) {
  struct timeval tv;
  uint64_t usecs;
  gettimeofday(&tv, NULL);
  usecs = tv.tv_usec + tv.tv_sec * 1000000;
  return usecs;
}

inline uint64_t get_hwtime(void) {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

#endif  // SCAL_UTIL_TIME_H_
