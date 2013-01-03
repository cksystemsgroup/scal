// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/random.h"

#include <stdio.h>

#include "util/threadlocals.h"

namespace {

// used for pseudorand
const uint32_t kA = 16807;
const uint32_t kM = 2147483647;
const uint32_t kQ = 127773;
const uint32_t kR = 2836;

}  // namespace

uint64_t pseudorand(void) {
  threadlocals_t *data = threadlocals_get();
  unsigned int lo, hi, test;
  hi = data->random_seed / kQ;
  lo = data->random_seed % kQ;
  test = kA * lo - kR * hi;
  if (test) {
    data->random_seed = test;
  } else {
    data->random_seed = test + kM;
  }
  return data->random_seed;
}

static inline uint64_t rdtsc(void) {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

uint64_t hwrand(void) {
  uint64_t rnd = (rdtsc() >> 6);
  return rnd;
}
