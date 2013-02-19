// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS

#include <gflags/gflags.h>
#include <stdint.h>

#include "benchmark/bfs/graph.h"
#include "datastructures/single_list.h"
#include "util/malloc.h"
#include "util/random.h"
#include "util/threadlocals.h"
#include "util/time.h"

DEFINE_string(prealloc_size, "1g", "tread local space that is initialized");
DEFINE_int64(root, -1, "root for BFS; -1: pseudorandom");
DEFINE_int64(end, -1, "end node; -1: pseudorandom");
DEFINE_bool(description, false, "print description");
DEFINE_bool(backtrace, false, "print backtrace");

namespace {

void analyze(uint64_t root_index, uint64_t end_index, Graph *g, SingleList<uint64_t> *q, uint64_t *execution_time) {
  uint64_t start_time = get_utime();
  uint64_t vertex_index;
  while (q->dequeue(&vertex_index)) {
    Vertex& cur_vertex = g->get(vertex_index);  
    for (uint64_t i = 0; i < cur_vertex.neighbors.size(); i++) {
      Vertex& neighbor = g->get(cur_vertex.neighbors[i]);
      if (neighbor.distance == Vertex::no_distance) {
        neighbor.distance = cur_vertex.distance + 1;
        neighbor.parent = vertex_index;
        if (cur_vertex.neighbors[i] == end_index) {
          goto AFTER_LOOP_ANALYZE;
        }
        q->enqueue(cur_vertex.neighbors[i]);
      }
    }
  }
AFTER_LOOP_ANALYZE:
  *execution_time = get_utime() - start_time;
  return;
}

void performance(uint64_t root_index, uint64_t end_index, Graph *g, SingleList<uint64_t> *q, uint64_t *execution_time) {
  uint64_t start_time = get_utime();
  uint64_t vertex_index;
  while (q->dequeue(&vertex_index)) {
    Vertex& cur_vertex = g->get(vertex_index);  
    for (uint64_t i = 0; i < cur_vertex.neighbors.size(); i++) {
      Vertex& neighbor = g->get(cur_vertex.neighbors[i]);
      if (neighbor.distance == Vertex::no_distance) {
        neighbor.distance = cur_vertex.distance + 1;
        if (cur_vertex.neighbors[i] == end_index) {
          goto AFTER_LOOP_PERFORMANCE;
        }
        q->enqueue(cur_vertex.neighbors[i]);
      }
    }
  }
AFTER_LOOP_PERFORMANCE:
  *execution_time = get_utime() - start_time;
  return;
}

}  // namespace

int main(int argc, char **argv) {
  std::string usage("sfp-analyzer [options] graph_file");
  google::SetUsageMessage(usage);
  uint32_t cmd_index = google::ParseCommandLineFlags(&
      argc, const_cast<char***>(&argv), true);
  if (cmd_index >= static_cast<uint32_t>(argc)) {
    google::ShowUsageWithFlags(google::GetArgv0());
    exit(EXIT_FAILURE);
  }
  const char *graph_file = argv[cmd_index];

  uint64_t tlsize = scal::human_size_to_pages(
      FLAGS_prealloc_size.c_str(), FLAGS_prealloc_size.size());
  scal::tlalloc_init(tlsize, true /* touch pages */);
  scal::ThreadContext::prepare(1);

  SingleList<uint64_t> *q = new SingleList<uint64_t>();
  Graph *g = Graph::from_graph_file(graph_file);

  uint64_t root_index;
  uint64_t end_index;
  if (FLAGS_root < 0) {
    root_index = scal::rand_range(0, g->size());
  } else {
    root_index = static_cast<uint64_t>(FLAGS_root);
  }
  if (FLAGS_end < 0) {
    end_index = scal::rand_range(0, g->size());
  } else {
    end_index = static_cast<uint64_t>(FLAGS_end);
  }

  Vertex& cur_vertex = g->get(root_index);
  cur_vertex.distance = 0;
  cur_vertex.parent = Vertex::no_parent;
  q->enqueue(root_index);

  uint64_t execution_time;
  if (FLAGS_backtrace) {
    analyze(root_index, end_index, g, q, &execution_time);
  } else {
    performance(root_index, end_index, g, q, &execution_time);
  }

  if (FLAGS_description) {
    printf("      size       root        end    sp  time [us]\n");
  }
  printf("%10" PRIu64 " %10" PRIu64 " %10" PRIu64 " %5" PRIu64 " %10" PRIu64 "\n",
      g->size(), root_index, end_index, g->get(end_index).distance, execution_time);
  if (FLAGS_backtrace) {
    Vertex& cur_vertex = g->get(end_index);
    printf("%lu ", end_index);
    while (true) {
      printf("-> %lu ", cur_vertex.parent);
      if (cur_vertex.parent == root_index) {
        break;
      }
      cur_vertex = g->get(cur_vertex.parent);
    }
    printf("\n");
  }

  return EXIT_SUCCESS;
}
