# Scal [![Build Status](https://drone.io/github.com/cksystemsgroup/scal/status.png)](https://drone.io/github.com/cksystemsgroup/scal/latest)


Scal is an open-source benchmarking framework that provides (1) software infrastructure for executing concurrent data structure algorithms, (2) workloads for benchmarking their performance and scalability, and (3) implementations of a large set of concurrent data structures.

Homepage: http://scal.cs.uni-salzburg.at <br>
Paper: [Scal: A Benchmarking Suite for Concurrent Data Structures](./paper.pdf)


## Data structures

|    Name   |  Semantics  |  Year | Ref |
|-----------|:-----:|:-----:|:-----:|
[Lock-based Singly-linked List Queue](./src/datastructures/lockbased_queue.h) | strict queue | 1968 | [[1]](#ref-knuth-1997)
[Michael Scott (MS) Queue](./src/datastructures/ms_queue.h) | strict queue | 1996 | [[2]](#ref-michael-1996)
[Flat Combining Queue](./src/datastructures/flatcombining_queue.h) | strict queue | 2010 | [[3]](#ref-hendler-2010)
[Wait-free Queue](./src/datastructures/wf_queue_ppopp12.h) | strict queue | 2012 | [[4]](#ref-kogan-2012)
[Linked Cyclic Ring Queue (LCRQ)](./src/datastructures/lcrq.h) | strict queue | 2013 | [[5]](#ref-morrison-2013)
[Timestamped (TS) Queue](./src/datastructures/ts_queue.h) | strict queue | 2015 | [[6]](#ref-dodds-2015)
[Cooperative TS Queue](./src/datastructures/cts_queue.h) | strict queue | 2015 | [[7]](#ref-haas-2015-1)
[Segment Queue](./src/datastructures/segment_queue.h) | k-relaxed queue | 2010 | [[8]](#ref-afek-2010)
[Random Dequeue (RD) Queue](./src/datastructures/random_dequeue_queue.h) | k-relaxed queue | 2010 | [[8]](#ref-afek-2010)
[Bounded Size k-FIFO Queue](./src/datastructures/boundedsize_kfifo.h) | k-relaxed queue, pool | 2013 | [[9]](#ref-kirsch-2013)
[Unbounded Size k-FIFO Queue](./src/datastructures/unboundedsize_kfifo.h) | k-relaxed queue, pool | 2013 | [[9]](#ref-kirsch-2013)
[b-RR](./src/datastructures/balancer_partrr.h) [Distributed](./src/datastructures/distributed_data_structure.h) [Queue (DQ)](./src/datastructures/ms_queue.h) | k-relaxed queue, pool | 2013 | [[10]](#ref-haas-2013)
[Least-Recently-Used (LRU)](./src/datastructures/lru_distributed_queue.h) [DQ](./src/datastructures/ms_queue.h) | k-relaxed queue, pool | 2013 | [[10]](#ref-haas-2013)
[Locally Linearizable](./src/datastructures/balancer_local_linearizability.h) [DQ](./src/datastructures/ms_queue.h) <br>([static](./src/datastructures/distributed_data_structure.h), [dynamic](./src/datastructures/dyn_distributed_data_structure.h)) | locally linearizable queue, pool | 2015 | [[11]](#ref-haas-2015-2)
[Locally Linearizable k-FIFO Queue](./src/datastructures/unboundedsize_kfifo.h) | locally linearizable queue <br> k-relaxed queue, pool  | 2015 | [[11]](#ref-haas-2015-2)
[Relaxed TS Queue](./src/datastructures/rts_queue.h) | quiescently consistent <br> queue (conjectured) | 2015 | [[7]](#ref-haas-2015-1)
[Lock-based Singly-linked List Stack](scal/src/datastructures/lockbased_stack.h) | strict stack | 1968 | [[1]](#ref-knuth-1997)
[Treiber Stack](./src/datastructures/treiber_stack.h) | strict stack | 1986 | [[12]](#ref-treiber-1986)
[Elimination-backoff Stack](./src/datastructures/elimination_backoff_stack.h) | strict stack | 2004 | [[13]](#ref-hendler-2004)
[Timestamped (TS) Stack](./src/datastructures/ts_stack.h) | strict stack | 2015 | [[6]](#ref-dodds-2015)
[k-Stack](./src/datastructures/kstack.h) | k-relaxed stack | 2013 | [[14]](#ref-henzinger-2013)
[b-RR](./src/datastructures/balancer_partrr.h) [Distributed](./src/datastructures/distributed_data_structure.h) [Stack (DS)](./src/datastructures/treiber_stack.h) | k-relaxed stack, pool | 2013 | [[10]](#ref-haas-2013)
[Least-Recently-Used (LRU)](./src/datastructures/lru_distributed_stack.h) [DS](./src/datastructures/treiber_stack.h) | k-relaxed stack, pool | 2013 | [[10]](#ref-haas-2013)
[Locally Linearizable](./src/datastructures/balancer_local_linearizability.h) [DS](./src/datastructures/treiber_stack.h) <br>([static](./src/datastructures/distributed_data_structure.h), [dynamic](./src/datastructures/dyn_distributed_data_structure.h)) | locally linearizable stack, pool | 2015 | [[11]](#ref-haas-2015-2)
[Locally Linearizable k-Stack](./src/datastructures/kstack.h) | locally linearizable stack <br> k-relaxed queue, pool | 2015 | [[11]](#ref-haas-2015-2)
[Timestamped (TS) Deque](./src/datastructures/ts_deque.h) | strict deque (conjectured) | 2015 | [[7]](#ref-haas-2015-1)
[d-RA](./src/datastructures/balancer_1random.h) [DQ](./src/datastructures/ms_queue.h) and [DS](./src/datastructures/treiber_stack.h) | strict pool | 2013 | [[10]](#ref-haas-2013)

## Dependencies

* pkg-config
* cmake
* [googletest](https://code.google.com/p/googletest "googletest")
* [gflags](https://code.google.com/p/gflags/ "gflags")
* [google-perftools](https://code.google.com/p/gperftools/ "google-perftools")

On debian (jessie) based systems:

    sudo apt-get install build-essential autoconf libtool google-perftools libgoogle-perftools-dev cmake libgtest-dev libgflags2 libgflags-dev

On Ubuntu (&ge; 12.04) based systems:

    sudo apt-get install build-essential autoconf libtool google-perftools libgoogle-perftools-dev cmake libgtest-dev
    wget https://gflags.googlecode.com/files/libgflags0_2.0-1_amd64.deb
    wget https://gflags.googlecode.com/files/libgflags-dev_2.0-1_amd64.deb
    sudo dpkg -i libgflags0_2.0-1_amd64.deb
    sudo dpkg -i libgflags-dev_2.0-1_amd64.deb

A similar script is executed on our [continuous integration test.](https://drone.io/github.com/cksystemsgroup/scal/admin)

## Building

Note: We switched from autotools to gyp for building the framework. The old
files are still present in the checkout but will be removed once everything is
converted.

This is as easy as

    tools/get_gyp.sh
    build/gyp/gyp --depth=. scal.gyp
    make
    BUILDTYPE=Release make

The debug and release builds reside in `out/`.

Additional data files, such as graph files, are available as submodule

    git submodule init
    git submodule update data

The resulting files reside in `data/`.

## Examples

### Producer/consumer

The most common parameters are:
* consumers: Number of consuming threads
* producers: Number of producing threads
* c: The computational workload (iterative pi calculation) between two data
  structure operations
* operations: The number of put/enqueue operations the should be performed by a
  producer

The following runs the Michael-Scott queue in a producer/consumer benchmark:

    ./prodcon-ms -producers=15 -consumers=15 -operations=100000 -c=250

And the same for the bounded-size k-FIFO queue:

    ./prodcon-bskfifo -producers=15 -consumers=15 -operations=100000 -c=250

And for Distributed Queue with a 1-random balancer:

    ./prodcon-dq-1random -producers=15 -consumers=15 -operations=100000 -c=250


Try `./prodcon-<data_structure> --help` to see the full list of available parameters.


## References

1. <a name="ref-knuth-1997"></a>D. E. Knuth. *The Art of Computer Programming, Volume 1 (3rd Ed.): Fundamental Algorithms.* Addison Wesley, Redwood City, CA, USA, 1997.

2. <a name="ref-michael-1996"></a>M.M. Michael and M.L. Scott. Simple, fast, and practical non-blocking and blocking concurrent queue algorithms. In *Proc. Symposium on Principles of Distributed Computing (PODC)*, pages 267–275. ACM, 1996.

3. <a name="ref-hendler-2010"></a>D. Hendler, I. Incze, N. Shavit, and M. Tzafrir. Flat combining and the synchronization-parallelism tradeoff. In *Proc. Symposium on Parallelism in Algorithms and Architectures (SPAA)*, pages 355–364. ACM, 2010.

4. <a name="ref-kogan-2012"></a>A. Kogan and E. Petrank. A methodology for creating fast wait-free data structures. In *Proc. Symposium on Principles and Practice of Parallel Programming (PPoPP)*, pages 141–150. ACM, 2012.

5. <a name="ref-morrison-2013"></a>A. Morrison and Y. Afek. Fast concurrent queues for x86 processors. In *Proc. Symposium on Principles and Practice of Parallel Programming (PPoPP)*, pages 103–112. ACM, 2013.

6. <a name="ref-dodds-2015"></a>M. Dodds, A. Haas, and C.M. Kirsch. A scalable, correct time-stamped stack. In *Proc. Symposium on Principles of Programming Languages (POPL)*, pages 233– 246. ACM, 2015.

7. <a name="ref-haas-2015-1"></a>A. Haas. *Fast Concurrent Data Structures Through Timestamping.* PhD thesis, University of Salzburg, Salzburg, Austria, 2015.

8. <a name="ref-afek-2010"></a>Y. Afek, G. Korland, and E. Yanovsky. Quasi-linearizability: Relaxed consistency for improved concurrency. In *Proc. Conference on Principles of Distributed Systems (OPODIS)*, pages 395–410. Springer, 2010.

9. <a name="ref-kirsch-2013"></a>C.M. Kirsch, M. Lippautz, and H. Payer. Fast and scalable, lock-free k-fifo queues. In *Proc. International Conference on Parallel Computing Technologies (PaCT)*, LNCS, pages 208–223. Springer, 2013.

10. <a name="ref-haas-2013"></a>A. Haas, T.A. Henzinger, C.M. Kirsch, M. Lippautz, H. Payer, A. Sezgin, and A. Sokolova. Distributed queues in shared memory—multicore performance and scalability through quantitative relaxation. In *Proc. International Conference on Computing Frontiers (CF)*. ACM, 2013.

11. <a name="ref-haas-2015-2"></a>A. Haas, T.A. Henzinger, A.Holzer, C.M. Kirsch, M. Lippautz, H. Payer, A. Sezgin, A. Sokolova, and H. Veith. Local linearizability. *CoRR*, abs/1502.07118, 2015.

12. <a name="ref-treiber-1986"></a>R.K. Treiber. Systems programming: Coping with parallelism. Technical Report RJ-5118, IBM Research Center, 1986.

13. <a name="ref-hendler-2004"></a>D. Hendler, N. Shavit, and L. Yerushalmi. A scalable lock-free stack algorithm. In *Proc. Symposium on Parallelism in Algorithms and Architectures (SPAA)*, pages 206–215. ACM, 2004.

14. <a name="ref-henzinger-2013"></a>T.A. Henzinger, C.M. Kirsch, H. Payer, A. Sezgin, and A. Sokolova. Quantitative relaxation of concurrent data structures. In *Proc. Symposium on Principles of Programming Languages (POPL)*, pages 317–328. ACM, 2013.


## License

Copyright (c) 2012-2013, the Scal Project Authors.
All rights reserved. Please see the AUTHORS file for details.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the Scal Project.
