// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <stdint.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/treiber_stack.h"

void* ds_new() {
  TreiberStack<uint64_t> *ts = new TreiberStack<uint64_t>();
  return static_cast<void*>(ts);
}

bool ds_put(void *ds, uint64_t item) {
  TreiberStack<uint64_t> *ts = static_cast<TreiberStack<uint64_t>*>(ds);
  return ts->push(item);
}

bool ds_get(void *ds, uint64_t *item) {
  TreiberStack<uint64_t> *ts = static_cast<TreiberStack<uint64_t>*>(ds);
  return ts->pop(item);
}

char* ds_get_stats(void) {
  return NULL;
}
