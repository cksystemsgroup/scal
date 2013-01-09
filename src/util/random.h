// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_RANDOM_H_
#define SCAL_UTIL_RANDOM_H_

#include <stdint.h>

uint64_t pseudorand(void);
uint64_t hwrand(void);

#endif  // SCAL_UTIL_RANDOM_H_
