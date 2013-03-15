#include <gflags/gflags.h>
#include <stdio.h>

#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/segment_queue.h"

DEFINE_uint64(k, 80, "segment size");

void* ds_new(void) {
  SegmentQueue<uint64_t> *sq = new SegmentQueue<uint64_t>(FLAGS_k);
  return static_cast<void*>(sq);
}

bool ds_put(void *ds, uint64_t item) {
  SegmentQueue<uint64_t> *sq = static_cast<SegmentQueue<uint64_t>*>(ds);
  bool ret =  sq->enqueue(item);
  return ret;
}

bool ds_get(void *ds, uint64_t *item) {
  SegmentQueue<uint64_t> *sq = static_cast<SegmentQueue<uint64_t>*>(ds);
  bool ret = sq->dequeue(item);
  return ret;
}

char* ds_get_stats(void) {
  return NULL;
}
