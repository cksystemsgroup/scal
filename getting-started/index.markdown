---
layout: default
title: Getting Started
---

# Getting Started

*August 2014*

The code of all data structure implementations is available on [GitHub][gh-scal].

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

* Michael-Scott queue \[1\]:
  * `src/datastructures/ms_queue.h`
* Lock-based queue:
  * `src/datastructures/lockbased_queue.h`
* Flat-combining queue \[2\]:
  * `src/datastructures/flatcombining_queue.h`
* Wait-free queue \[3, 4\]:
  * `src/datastructures/wf_queue_ppopp11.h`: wait-free queue as described in \[3\]
  * `src/datastructures/wf_queue_ppopp12.h`: wait-free queue as described in \[4\]
* Distributed queue \[5\]:
  * `src/datastructures/distributed_queue.h`: generic distributed queue skeleton
  * `src/datastructures/balancer_1random.h`: a load balancer which selects a random partial queue
  * `src/datastructures/balancer_id.h`: a load balancer which uses the id of the executing thread to select a partial queue
  * `src/datastrcutures/balancer_partrr.h`: a round robin load balancer. Multiple round robin counters can be used, each thread is statically assigned to one round robin counter
* k-FIFO queue \[6\]:
  * `src/datastructures/boundedsize_kfifo.h`: bounded-size k-FIFO queue
  * `src/datastructures/unboundedsize_kfifo.h`: unbounded-size k-FIFO queue
* Random dequeue queue \[7\]:
  * `src/datastructures/random_dequeue_queue.h`
* TS stack \[8\]:
  * `src/datastructures/ts_stack.h`: TS stack skeleton
  * `src/datastructures/ts_timestamp.h`: timestamping algorithms
  * `src/datastructures/ts_stack_buffer.h`: TS buffer of the TS stack
* TS queue \[8\]:
  * `src/datastructures/ts_queue.h`: TS queue skeleton
  * `src/datastructures/ts_timestamp.h`: timestamping algorithms
  * `src/datastructures/ts_queue_buffer.h`: TS buffer of the TS queue
* TS deque \[8\]:
  * `src/datastructures/ts_deque.h`: TS deque skeleton
  * `src/datastructures/ts_timestamp.h`: timestamping algorithms
  * `src/datastructures/ts_deque_buffer.h`: TS buffer of the TS deque
* DTS queue \[9\]:
  * `src/datastructures/dts_queue.h`
* Treiber stack \[10\]:
  * `src/datastructures/treiber_stack.h`
* k-Stack \[11\]:
  * `src/datastructures/kstack.h`

### Framework: `src/util/`

Most (if not all) of the actual framework stuff, like atomics, a scalable
relaxed allocator, or threadlocal storage can be found in here.

### References:

1. M. Michael, M. Scott. Simple, fast, and practical non-blocking and blocking concurrent queue algorithms. In Proc. PODC, 1996.
2. D. H. I. Incze, N. Shavit, M. Tzafrir. Flat combining and the synchronization-parallelism tradeoff. In Proc. SPAA, 2010.
3. A. Kogan, E. Petrank. Wait-free queues with multiple enqueuers and dequeuers. In Proc. PPoPP, 2011.
4. A. Kogan, E. Petrank. A methodology for creating fast wait-free data structures. In Proc. PPoPP, 2012.
5. A. Haas, T. A. Henzinger, C. M. Kirsch, M. Lippautz, H. Payer, A. Sezgin, A. Sokolova. Distributed queues in shared memory - multicore performance and scalability through quantitative relaxation. In Proc. CF, 2013.
6. C. M. Kirsch, M. Lippautz, H. Payer. Fast and scalable, lock-free k-FIFO queues. In Proc. PaCT, 2013.
7. Y. Afek, G. Korland, E. Yanovsky. Quasi-linearizability: Relaxed consistency for improved concurrency. In Proc. OPODIS, 2010.
8. M. Dodds, A. Haas, C. M. Kirsch. Fast concurrent data structures through explicit timestamping. Technical report TS-2014-02, Department of Computer Sciences, University of Salzburg, 2014.
9. M. Dodds, A. Haas, C. M. Kirsch. Fairness vs. linearizability in a concurrent FIFO queue. Presented at DMTM, 2014.
10. R. Treiber. Systems programming: Coping with parallelism. Technical report RJ5118, IBM Almaden Research Center, 1986.
11. T. A. Henzinger, C. M. Kirsch, H. Payer, A. Sezgin, and A. Sokolova. Quantitative relaxation of concurrent data structures. In Proc. POPL, 2013.
