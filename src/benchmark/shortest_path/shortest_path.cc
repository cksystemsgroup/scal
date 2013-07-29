// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends
#define __STDC_LIMIT_MACROS

#include <gflags/gflags.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "benchmark/common.h"
#include "benchmark/std_glue/std_pipe_api.h"
#include "datastructures/pool.h"
#include "util/malloc.h"
#include "util/operation_logger.h"
#include "util/random.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/workloads.h"
#include "util/platform.h"
#include <assert.h>
#include <atomic>

DEFINE_string(prealloc_size, "1g", "tread local space that is initialized");
DEFINE_uint64(threads, 1, "number of threads");
DEFINE_uint64(height, 100, "height of the grid");
DEFINE_uint64(width, 10000, "width of the grid");
DEFINE_uint64(start, 1, "starting node; Node 0 is not valid");
DEFINE_int64(end, -1, "end node");
//DEFINE_uint64(c, 5000, "computational workload");
DEFINE_bool(print_summary, true, "print execution summary");
DEFINE_bool(log_operations, false, "log invocation/response/linearization "
                                   "of all operations");

#define CACHE_FACTOR 16 

using scal::Benchmark;

inline int get_index(int index) {
  return index * CACHE_FACTOR;
}

inline uint64_t get_left(uint64_t index, uint64_t width, uint64_t height) {

  if (index % width == 0) {
    return UINT64_MAX;
  } else {
    return index - 1;
  }
}

inline uint64_t get_right(uint64_t index, uint64_t width, uint64_t height) {

  uint64_t result = index + 1;

  if (result % width == 0) {
    return UINT64_MAX;
  } else {
    return result;
  }
}

inline uint64_t get_above(uint64_t index, uint64_t width, uint64_t height) {

  if (index < width) {
    return UINT64_MAX;
  } else {
    return index - width;
  }
}

inline uint64_t get_below(uint64_t index, uint64_t width, uint64_t height) {

  uint64_t result = index + width;

  if (result >= width * height) {
    return UINT64_MAX;
  } else {
    return result;
  }
}

inline bool is_valid(uint64_t index) {
  return index != UINT64_MAX;
}


class ShortestPathBench : public Benchmark {
 public:
  ShortestPathBench(uint64_t num_threads,
               uint64_t thread_prealloc_size,
               void *data)
                   : Benchmark(num_threads,
                               thread_prealloc_size,
                               data) {}
  int check_races(void);
  uint64_t get_distance(uint64_t index);
  bool set_distance(uint64_t index, uint64_t value);
 protected:
  void bench_func(void);

 private:
void analyze_race(
    uint8_t *marking, uint64_t node, uint64_t neighbor, uint64_t *worst_race, uint64_t *result);

int is_bad_race(int neighbor, uint64_t distance);

};

uint64_t g_num_threads;
uint8_t *g_up;
uint8_t *g_down;
uint8_t *g_right;
std::atomic<uint64_t>* test;
uint64_t *g_marking;

inline uint64_t ShortestPathBench::get_distance(uint64_t index) {
  return test[get_index(index)].load(std::memory_order_relaxed);
//  return result;
//  return test[get_index(index)];
//  return atomic_load(&test[get_index(index)]);
//  return atomic_load_explicit(&test[get_index(index)], std::memory_order_seq_cst);
//  return atomic_load_explicit(&test[get_index(index)], std::memory_order_acquire);

//  return g_marking[get_index(index)];
}

inline bool ShortestPathBench::set_distance(uint64_t index, uint64_t value) {
  uint64_t expected = 0;
//  test[get_index(index)] = value;
//  test[get_index(index)].store(value, std::memory_order_relaxed);
  return test[get_index(index)].compare_exchange_strong(
      expected, value, std::memory_order_relaxed, std::memory_order_relaxed);
//  atomic_compare_exchange_strong_explicit(&test[get_index(index)], &expected, value,
//      std::memory_order_seq_cst, std::memory_order_seq_cst);
//  atomic_store(&test[get_index(index)], value);
//  atomic_store_explicit(&test[get_index(index)], value, std::memory_order_seq_cst);
//  atomic_store_explicit(&test[get_index(index)], value, std::memory_order_release);
//  return __sync_bool_compare_and_swap(&g_marking[get_index(index)], 0, value);
//  g_marking[get_index(index)] = value;
//  return true;
}

inline int ShortestPathBench::is_bad_race(int neighbor, uint64_t distance) {
  if (is_valid(neighbor)) {
    uint64_t distance_neighbor = get_distance(neighbor);
    if (distance > distance_neighbor) {
      return distance - distance_neighbor;
    }
  }
  return 0;
}

int max(int v1, int v2) {
//  return v1 + v2;
  if (v1 > v2) {
    return v1;
  }

  return v2;
}

void ShortestPathBench::analyze_race(
    uint8_t *marking, uint64_t node, uint64_t neighbor, uint64_t *worst_race, uint64_t *result) {

  if (marking[get_index(node)] != 0) {
    uint64_t race = is_bad_race(neighbor, get_distance(node));
    if (1 < race) {
      *worst_race = max(*worst_race, race);
      (*result)++;
    }
  }
}

int ShortestPathBench::check_races() {
  // Check if there has been a race for some nodes.

  uint64_t result = 0;
  uint64_t worst_race = 0;

  for (int i = 2; i < FLAGS_height * FLAGS_width; i++) {

    int neighbor_above = get_above(i, FLAGS_width, FLAGS_height);
    int neighbor_below = get_below(i, FLAGS_width, FLAGS_height);
    int neighbor_left =  get_left (i, FLAGS_width, FLAGS_height);

    bool ok = false;


    if (is_valid(neighbor_above)) {
      if (g_down[get_index(i)] == 1) {
        if (get_distance(neighbor_above) + 1 == get_distance(i)) {
          analyze_race(g_up, i, neighbor_above, &worst_race, &result);
          analyze_race(g_right, i, neighbor_left, &worst_race, &result);
          ok = true;
        }
      }
    }

    if (is_valid(neighbor_below)) {
      if (g_up[get_index(i)] == 1) {
        if (get_distance(neighbor_below) + 1 == get_distance(i)) {
          analyze_race(g_down, i, neighbor_above, &worst_race, &result);
          analyze_race(g_right, i, neighbor_left, &worst_race, &result);
          ok = true;
        }
      }
    }

    if (is_valid(neighbor_left)) {
      if (g_right[get_index(i)] == 1) {
        if (get_distance(neighbor_left) + 1 == get_distance(i)) {
          analyze_race(g_up, i, neighbor_below, &worst_race, &result);
          analyze_race(g_down, i, neighbor_above, &worst_race, &result);
          ok = true;
        }
      }
    }

    if (get_distance(i) == 0) {
      ok = true;
    }

    if (!ok) {
      printf("distance: %" PRIu64 "; above: %" PRIu64 "; below: %" PRIu64 "; left: %" PRIu64 "\n",
          get_distance(i), 
          get_distance(neighbor_above),
          get_distance(neighbor_below),
          get_distance(neighbor_left));

    }

    assert(ok);
  }

  printf("worst race: %" PRIu64 "\n", worst_race);
  return result;
}

int main(int argc, const char **argv) {
  std::string usage("Shortest path micro benchmark.");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);

  uint64_t tlsize = scal::human_size_to_pages(FLAGS_prealloc_size.c_str(),
                                              FLAGS_prealloc_size.size());

  // Init the main program as executing thread (may use rnd generator or tl
  // allocs).
  g_num_threads = FLAGS_threads;
  scal::tlalloc_init(tlsize, true /* touch pages */);
  //threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

 // if (FLAGS_log_operations) {
 //   scal::StdOperationLogger::prepare(g_num_threads + 1,
 //                                     FLAGS_operations +100000);
 // }

  void *ds = ds_new();
  Pool<uint64_t> *queue = static_cast<Pool<uint64_t>*>(ds);

//  g_up = static_cast<uint8_t*>(scal::malloc_aligned(
//        FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR, scal::kPageSize));
//  memset(g_up, 0, FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR);
//
//  g_down = static_cast<uint8_t*>(scal::malloc_aligned(
//        FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR, scal::kPageSize));
//  memset(g_down, 0, FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR);
//
//  g_right = static_cast<uint8_t*>(scal::malloc_aligned(
//        FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR, scal::kPageSize));
//  memset(g_right, 0, FLAGS_height * FLAGS_width * sizeof(uint8_t) * CACHE_FACTOR);
//
  g_marking = static_cast<uint64_t*>(scal::malloc_aligned(
        FLAGS_height * FLAGS_width * sizeof(uint64_t) * CACHE_FACTOR, scal::kPageSize));
  memset(g_marking, 0, FLAGS_height * FLAGS_width * sizeof(uint64_t) * CACHE_FACTOR);


  test = static_cast<std::atomic<uint64_t>*>(scal::malloc_aligned(
        FLAGS_height *FLAGS_width * sizeof(std::atomic<uint64_t>) * CACHE_FACTOR, scal::kPageSize));

  memset(test, 0, FLAGS_height * FLAGS_width * sizeof(std::atomic<uint64_t>) * CACHE_FACTOR);

  queue->put(FLAGS_start);

  if (FLAGS_end < 0) {
    FLAGS_end = FLAGS_height * FLAGS_width - 1;
  }

  ShortestPathBench *benchmark = new ShortestPathBench(
      g_num_threads,
      tlsize,
      ds);
  benchmark->set_distance(FLAGS_start, 1);

  benchmark->run();

  if (FLAGS_log_operations) {
    scal::StdOperationLogger::print_summary();
  }

  if (FLAGS_print_summary) {
    uint64_t exec_time = benchmark->execution_time();

//    int num_races = benchmark->check_races();

    char buffer[1024] = {0};
    uint32_t n = snprintf(buffer, sizeof(buffer), "threads: %" PRIu64 " runtime: %" PRIu64 " costs: %" PRIu64 " ds_stats: ",
        FLAGS_threads,
        exec_time,
        benchmark->get_distance(FLAGS_end)
        );

//    uint32_t n = snprintf(buffer, sizeof(buffer), "threads: %" PRIu64 " runtime: %" PRIu64 " costs: %" PRIu64 " races: %d ds_stats: ",
//        FLAGS_threads,
//        exec_time,
//        benchmark->get_distance(FLAGS_end),
//        num_races);

    if (n != strlen(buffer)) {
      fprintf(stderr, "%s: error: failed to create summary string\n", __func__);
      abort();
    }
    char *ds_stats = ds_get_stats();
    if (ds_stats != NULL) {
      if (n + strlen(ds_stats) >= 1023) {  // separating space + '\0'
        fprintf(stderr, "%s: error: strings too long\n", __func__);
        abort();
      }
      strcat(buffer, " ");
      strcat(buffer, ds_stats);
    }
    printf("%s\n", buffer);
  }
  return EXIT_SUCCESS;
}

void ShortestPathBench::bench_func(void) {

  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  
  uint64_t node;
  bool ok;

  while (get_distance(FLAGS_end) == 0) {

    ok = ds->get(&node);
    if (!ok) {
      continue;
    }

    int distance = get_distance(node);
    assert(distance != 0);

    int neighbor_above = get_above(node, FLAGS_width, FLAGS_height);
    int neighbor_below = get_below(node, FLAGS_width, FLAGS_height);
    int neighbor_right = get_right(node, FLAGS_width, FLAGS_height);

    if (is_valid(neighbor_right)) {
      if (get_distance(neighbor_right) == 0) {
//        if (__sync_bool_compare_and_swap(&g_marking[get_index(neighbor_right)], 0, distance + 1)) {
        if (set_distance(neighbor_right, distance + 1)) {
//          g_right[get_index(neighbor_right)] = 1;
          if (!ds->put(neighbor_right)) {
            // We should always be able to insert an item.
            fprintf(stderr, "%s: error: put operation failed.\n", __func__);
            abort();
          }
        }
      }
    }

    if (is_valid(neighbor_below)) {

      if (get_distance(neighbor_below) == 0) {
//        if (__sync_bool_compare_and_swap(&g_marking[get_index(neighbor_below)], 0, distance + 1)) {
        if (set_distance(neighbor_below, distance + 1)) {
//          g_down[get_index(neighbor_below)] = 1;
          if (!ds->put(neighbor_below)) {
            // We should always be able to insert an item.
            fprintf(stderr, "%s: error: put operation failed.\n", __func__);
            abort();
          }
        }
      }
    }

    if (is_valid(neighbor_above)) {
      if (get_distance(neighbor_above) == 0) {
        if (set_distance(neighbor_above, distance + 1)) {
//          g_up[get_index(neighbor_above)] = 1;
          if (!ds->put(neighbor_above)) {
            // We should always be able to insert an item.
            fprintf(stderr, "%s: error: put operation failed.\n", __func__);
            abort();
          }
        }
      }
    }
  }

}
