// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

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
#ifdef DEBUG_RANDOM
  printf("%s: tls: %p; thread_id: %lu; random_nr (new seed): %lu\n",
         __func__, data, data->thread_id, data->random_seed);
#endif  // DEBUG_RANDOM
  return data->random_seed;
}

static inline uint64_t rdtsc(void) {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

uint64_t hwrand(void) {
  uint64_t rnd = (rdtsc() >> 6);
#ifdef DEBUG_RANDOM
  printf("%s: random_nr: %lu\n", __func__, rnd);
#endif  // DEBUG_RANDOM
  return rnd;
}
