// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_BENCHMARK_STD_PIPE_API_H_
#define SCAL_BENCHMARK_STD_PIPE_API_H_

#include <inttypes.h>

extern uint64_t g_num_threads;

extern void* ds_new(void);
extern bool ds_put(void *ds, uint64_t val);
extern bool ds_get(void *ds, uint64_t *val);
extern char* ds_get_stats(void);

#endif  // SCAL_BENCHMARK_STD_PIPE_API_H_
