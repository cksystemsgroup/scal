// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_BENCHMARK_STD_PIPE_API_H_
#define SCAL_BENCHMARK_STD_PIPE_API_H_

#include <stdint.h>

extern uint64_t g_num_threads;

extern void* ds_new(void);
extern bool ds_put(void *ds, uint64_t val);
extern bool ds_get(void *ds, uint64_t *val);
extern char* ds_get_stats(void);

#endif  // SCAL_BENCHMARK_STD_PIPE_API_H_
