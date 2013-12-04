---
layout: default
title: Getting Started
---

# Getting Started

*December 2013*

This basic documentation should help you getting started with the code that can
be found on [GitHub][gh-scal].

[gh-scal]: https://github.com/cksystemsgroup/scal

## Source Tree Overview

All relevant source files can be found within the `src/` directory.

### Benchmarks: `src/benchmark/`

`benchmark/prodcon/`
: A producer/consumer based benchmark.

`benchmark/bfs/`
: A breadth first search benchmark using a single work queue to operate on a
graph.

`benchmark/std_glue/`
: Each data structure comes with a *glue files* that specifies how to perform
operations of the benchmark on the data structure. (Originally scal was coded in
C, leaving still some bits and pieces behind.)

### Data Structures: `src/datastructures/`

Holds all (!) data structures as `.h` includes. This is necessary because all of
them make use of C++ generics (templates).

**An example**:

The well-known Michael-Scott queue can be found in
`src/datastructures/ms_queue.h`.

### Framework: `src/util/`

Most (if not all) of the actual framework stuff, like atomics, a scalable
relaxed allocator, or threadlocal storage can be found in here.

