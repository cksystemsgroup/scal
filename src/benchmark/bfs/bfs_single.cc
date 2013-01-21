// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gflags/gflags.h>

#include "benchmark/bfs/graph.h"
#include "datastructures/single_list.h"
#include "util/malloc.h"
#include "util/random.h"
#include "util/threadlocals.h"
#include "util/time.h"

DEFINE_string(prealloc_size, "1g", "tread local space that is initialized");
DEFINE_string(graph_file, "data/graph-test.graph", "graph file for BFS");

int main(int argc, char **argv) {
  std::string usage("BFS graph benchmark.");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  uint64_t tlsize = scal::human_size_to_pages(FLAGS_prealloc_size.c_str(),
                                              FLAGS_prealloc_size.size());
  scal::tlalloc_init(tlsize, true /* touch pages */);
  threadlocals_init();

  SingleList<uint64_t> *q = new SingleList<uint64_t>();
  Graph *g = Graph::from_graph_file(FLAGS_graph_file.c_str());
  uint64_t vertex_index;
  Vertex *cur_vertex;
  Vertex *neighbor;

  vertex_index = pseudorand() % g->num_vertices();
  cur_vertex = g->get(vertex_index);
  cur_vertex->distance = 0;
  q->enqueue(vertex_index);
  uint64_t start_time = get_utime();
  while (q->dequeue(&vertex_index)) {
    cur_vertex = g->get(vertex_index);  
    for (uint64_t i = 0; i < cur_vertex->len_neighbors; i++) {
      neighbor = g->get(cur_vertex->neighbors[i]);
      if (neighbor->distance == Vertex::distance_not_set) {
        neighbor->distance = cur_vertex->distance + 1;
        q->enqueue(cur_vertex->neighbors[i]);
      }
    }
  }
  uint64_t execution_time = get_utime() - start_time;
  printf("%lu %lu\n", g->num_vertices(), execution_time);


  return EXIT_SUCCESS;
}
