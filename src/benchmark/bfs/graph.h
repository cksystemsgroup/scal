#ifndef SCAL_BENCHMARK_BFS_GRAPH_H
#define SCAL_BENCHMARK_BFS_GRAPH_H

#include <stdint.h>

#include <limits>

struct Vertex {
  static const uint64_t distance_not_set = std::numeric_limits<uint64_t>::max();

  uint64_t *neighbors;
  uint64_t len_neighbors;
  uint64_t distance;
};

class Graph {
 public:
  static Graph* from_graph_file(const char* graph_file);
  
  inline uint64_t num_vertices() {
    return num_vertices_;
  }

  inline Vertex* get(uint64_t index) {
    return vertices_[index];
  }

 private:
  Graph() { };
  Graph(Graph& cpy) { };

  uint64_t num_vertices_;
  Vertex **vertices_;
};

#endif  // SCAL_BENCHMARK_BFS_GRAPH_H
