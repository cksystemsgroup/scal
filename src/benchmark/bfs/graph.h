// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_BENCHMARK_BFS_GRAPH_H
#define SCAL_BENCHMARK_BFS_GRAPH_H

#include <stdint.h>

#include <limits>

struct Vertex {
  static const uint64_t no_distance = std::numeric_limits<uint64_t>::max();

  uint64_t *neighbors;
  uint64_t len_neighbors;
  uint64_t distance;
};

class Graph {
 public:
  static Graph* from_graph_file(const char* graph_file);
  
  inline uint64_t size() {
    return num_vertices_;
  }

  inline Vertex& get(uint64_t index) {
    return *vertices_[index];
  }

 private:
  Graph() {}
  Graph(const Graph &cpy) {}

  uint64_t num_vertices_;
  Vertex **vertices_;
};

#endif  // SCAL_BENCHMARK_BFS_GRAPH_H
