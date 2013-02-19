// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include "benchmark/bfs/graph.h"

#include <assert.h>
#include <stdlib.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#include "util/malloc.h"

namespace {

void invalid_graph_file(const char *fn) {
  fprintf(stderr, "%s: invalid graph file\n", fn);
  exit(EXIT_FAILURE);
}

void invalid_mtx_file(const char* fn) {
  fprintf(stderr, "%s: invalid matrix market format file\n", fn);
  exit(EXIT_FAILURE);
}

bool inline is_comment(const std::string &source) {
  if (source.find_first_of("%") == 0) {
    return true;
  }
  return false;
}

std::vector<uint64_t> inline tokenize(const std::string &source,
                                      const char *delimiter = " ",
                                      bool keep_empty = false) {
  std::vector<uint64_t> results;
  size_t prev = 0;
  size_t next = 0;
  uint64_t num;
  while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
    if (keep_empty || (next - prev != 0)) {
      num = atol(source.substr(prev, next - prev).c_str());
      if (num == 0) {
        invalid_graph_file(__func__);
      }
      // We use zero based indices and node ids.
      results.push_back(num - 1);
    }
    prev = next + 1;
  }
  if (prev < source.size()) {
    num = atol(source.substr(prev).c_str());
    if (num == 0) {
      invalid_graph_file(__func__);
    }
    // Again, we use zero based indices and node ids.
    results.push_back(num - 1);
  }
  return results;
}

}  // namespace

// We assume a correctly formated mtx file o type:
// matrix coordinate pattern general
Graph* Graph::from_mtx_file(const char* graph_file) {
  Graph *g = new Graph();
  FILE* infile;
  char line_buffer[BUFSIZ];

  infile = fopen(graph_file, "rt");
  if (!infile) {
    fprintf(stderr, "%s: unable to open file %s\n", __func__, graph_file);
    exit(EXIT_FAILURE);
  }

  bool firstline = true;
  while (fgets(line_buffer, sizeof(line_buffer), infile)) {
    std::string line(line_buffer);
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
    if (is_comment(line)) {
      continue;
    }
    if (firstline) {
      // syntax of first line:
      // #rows #cols #entries
      // for our case it should be #rows=#cols
      firstline = false;
      std::vector<uint64_t> tokens = tokenize(line, " ");
      if (tokens.size() < 1) {
        invalid_mtx_file(__func__);
      }
      g->vertices_ = new Vertex*[tokens[0]];
      g->num_vertices_ = tokens[0];
      for (uint64_t i = 0; i < tokens[0]; i++) {
        g->vertices_[i] = scal::get<Vertex>(4 * 128);
        g->vertices_[i]->distance = Vertex::no_distance;
        g->vertices_[i]->parent = Vertex::no_parent;
      }
      continue;
    }
    std::vector<uint64_t> tokens = tokenize(line, " ");
    assert((tokens[0] - 1) < g->num_vertices_);
    Vertex *v = g->vertices_[tokens[0] - 1];
    v->neighbors.push_back(tokens[1] - 1);
  }

  fclose(infile);
  return g;
}

Graph* Graph::from_graph_file(const char* graph_file) {
  Graph *g = new Graph();
  FILE* infile;
  char line_buffer[BUFSIZ];

  infile = fopen(graph_file, "rt");
  if (!infile) {
    fprintf(stderr, "%s: unable to open file %s\n", __func__, graph_file);
    exit(EXIT_FAILURE);
  }

  bool firstline = true;
  uint64_t id = 0;
  while (fgets(line_buffer, sizeof(line_buffer), infile)) {
    std::string line(line_buffer);
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
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
        g->vertices_[i] = scal::get<Vertex>(4 * 128);
        g->vertices_[i]->distance = Vertex::no_distance;
        g->vertices_[i]->parent = Vertex::no_parent;
      }
      continue;
    } 
    //std::vector<uint64_t> tokens = tokenize(line, " ");
    Vertex *v = g->vertices_[id];
    v->neighbors = tokenize(line, " ");

    /*
    if (tokens.size() > 0) {
      v->neighbors = static_cast<uint64_t*>(calloc(
          tokens.size(), sizeof(uint64_t)));
      v->len_neighbors = tokens.size();
      for (uint64_t i = 0; i < tokens.size(); i++) {
        v->neighbors[i] = tokens[i];
      }
    } else {
      v->len_neighbors = 0;
      v->neighbors = NULL;
    }
    */
    id++;
  }

  fclose(infile);
  return g;
}
