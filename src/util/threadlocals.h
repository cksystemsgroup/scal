// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_THREADLOCALS_H_
#define SCAL_UTIL_THREADLOCALS_H_

#include <inttypes.h>

struct threadlocals_t {
  uint64_t thread_id;
  unsigned int random_seed;
  void *data;
};

void threadlocals_init(void);
threadlocals_t* threadlocals_get(void);
threadlocals_t* threadlocals_get(uint64_t);
uint64_t threadlocals_num_threads(void);

#endif  // SCAL_UTIL_THREADLOCALS_H_
