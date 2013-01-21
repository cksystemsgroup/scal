#include "benchmark/bfs/graph.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#include "util/malloc.h"

namespace {

void invalid_graph_file(const char *fn) {
  fprintf(stderr, "%s: invalid graph file", fn);
  exit(EXIT_FAILURE);
}

bool inline is_comment(const std::string &source) {
  if (source.find_first_of("%") == 0) {
    return true;
  }
  return false;
}

std::vector<uint64_t> inline tokenize(
    const std::string &source, const char *delimiter = " ", bool keep_empty = false) {
  std::vector<uint64_t> results;
  size_t prev = 0;
  size_t next = 0;
  while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
    if (keep_empty || (next - prev != 0)) {
      uint64_t num = atol(source.substr(prev, next - prev).c_str());
      if (num == 0) {
        invalid_graph_file(__func__);
      }
      results.push_back(num - 1);  // we are zero-based (contrary to graph format)
    }
    prev = next + 1;
  }
  if (prev < source.size()) {
    uint64_t num = atol(source.substr(prev).c_str());
    if (num == 0) {
      invalid_graph_file(__func__);
    }
    results.push_back(num - 1);  // we are zero-based (contrary to graph format)
  }
  return results;
}

}  // namespace

Graph* Graph::from_graph_file(const char* graph_file) {
  Graph *g = new Graph();
  std::ifstream infile(graph_file);
  std::string line;
  bool firstline = true;
  uint64_t id = 0;
  while(std::getline(infile, line)) {
    if (is_comment(line)) {
      continue;
    }
    if (firstline) {
      // syntax of first line:
      // #vertices #edges ...
      firstline = false;
      std::vector<uint64_t> tokens = tokenize(line, " ");
      if (tokens.size() < 1) {
        invalid_graph_file(__func__);
      }
      tokens[0]++;  // first line is not based on 1...
      g->vertices_ = new Vertex*[tokens[0]];
      g->num_vertices_ = tokens[0];
      for (uint64_t i = 0; i < tokens[0]; i++) {
        g->vertices_[i] = scal::get<Vertex>(128);
        g->vertices_[i]->distance = Vertex::distance_not_set;
      }
      continue;
    } 
    std::vector<uint64_t> tokens = tokenize(line, " ");
    Vertex *v = g->vertices_[id];
    if (tokens.size() > 0) {
      v->neighbors = static_cast<uint64_t*>(scal::calloc_aligned(tokens.size(), sizeof(uint64_t), 128));
      v->len_neighbors = tokens.size();
      for (uint64_t i = 0; i < tokens.size(); i++) {
        v->neighbors[i] = tokens[i];
      }
    } else {
      v->len_neighbors = 0;
      v->neighbors = NULL;
    }
    id++;
  }
  return g;
}
