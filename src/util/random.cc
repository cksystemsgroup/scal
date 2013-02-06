// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "util/random.h"

#include <stdio.h>

#include "util/threadlocals.h"

namespace {

// Used for pseudorand
const uint32_t kA = 16807;
const uint32_t kM = 2147483647;
const uint32_t kQ = 127773;
const uint32_t kR = 2836;

// Used for hwrand
inline uint64_t rdtsc(void) {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

}  // namespace

// Schrage minimum standard PRNG. Assumes int to be 32 bits.
// Range: [0,kM]
uint64_t pseudorand() {
  int32_t seed = scal::ThreadContext::get().random_seed();
  uint32_t hi = seed / kQ;
  uint32_t lo = seed % kQ;
  seed = kA * lo - kR * hi;
  if (seed < 0) {
    seed += kM;
  }
  scal::ThreadContext::get().set_random_seed(seed);
  return seed;
}

uint64_t pseudorandrange(uint32_t min, uint32_t max) {
  uint32_t range = max - min;
  return (static_cast<double>(pseudorand()) / (static_cast<double>(kM) + 1.0))
      * static_cast<double>(range) + min;
}


uint64_t hwrand(void) {
  uint64_t rnd = (rdtsc() >> 6);
  return rnd;
}

namespace scal {

void srand(uint32_t seed) {
  ThreadContext::get().set_random_seed(seed);
}

}  // namespace scal

