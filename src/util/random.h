// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_RANDOM_H_
#define SCAL_UTIL_RANDOM_H_

#include <stdint.h>


uint64_t pseudorand();
uint64_t pseudorandrange(uint32_t min, uint32_t max);
uint64_t hwrand();

namespace scal {

const uint32_t kRandMax = 2147483647;

inline uint64_t rand() {
  return pseudorand();
}

inline uint64_t randrange(uint32_t min, uint32_t max) {
  return pseudorandrange(min, max);
}

void srand(uint32_t seed);

inline uint64_t hwrand() {
  return ::hwrand();
}

}  // namespace scal

#endif  // SCAL_UTIL_RANDOM_H_
