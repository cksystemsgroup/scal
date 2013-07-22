# We'll add the programs throughout the Makefile.
bin_PROGRAMS =
noinst_PROGRAMS =
TESTS = 

AM_CPPFLAGS = -Werror -mcx16 -m64  -std=c++11 \
        @CC_OPTIMIZATION@ \
        -I${top_srcdir}/src \
        -ffast-math \
        -fno-builtin-malloc \
        -fno-builtin-calloc \
        -fno-builtin-realloc \
        -fno-builtin-free \
        -fno-omit-frame-pointer \
	@GFLAGS_CFLAGS@

AM_LDFLAGS = -Wl,--as-needed 
LDADD = @GFLAGS_LIBS@

DATASTRUCTURE_INCLUDES = \
	src/datastructures/balancer_1random.h \
	src/datastructures/balancer_2random.h \
	src/datastructures/balancer.h \
	src/datastructures/balancer_id.h \
	src/datastructures/balancer_partrr.h \
	src/datastructures/boundedsize_kfifo.h \
	src/datastructures/distributed_queue.h \
	src/datastructures/distributed_queue_interface.h \
	src/datastructures/partial_pool_interface.h \
	src/datastructures/flatcombining_queue.h \
	src/datastructures/kstack.h \
	src/datastructures/lockbased_queue.h \
	src/datastructures/ms_queue.h \
	src/datastructures/ah_queue.h \
	src/datastructures/enq_pool.h \
	src/datastructures/segment_queue.h \
	src/datastructures/pool.h \
	src/datastructures/queue.h \
	src/datastructures/random_dequeue_queue.h \
	src/datastructures/single_list.h \
	src/datastructures/stack.h \
	src/datastructures/treiber_stack.h \
	src/datastructures/unboundedsize_kfifo.h \
	src/datastructures/wf_queue_ppopp11.h \
	src/datastructures/wf_queue_ppopp12.h 

noinst_HEADERS = \
	$(DATASTRUCTURE_INCLUDES) \
	src/benchmark/common.h \
	src/benchmark/std_glue/std_pipe_api.h \
	src/util/atomic_value.h \
	src/util/atomic_value128.h \
	src/util/atomic_value64_base.h \
	src/util/atomic_value64_offset.h \
	src/util/atomic_value64_no_offset.h

UTIL_OBJS = \
	src/util/atomic_value128.h \
	src/util/atomic_value128_unittest-malloc.o \
	src/util/atomic_value64_base.h \
	src/util/atomic_value64_no_offset.h \
	src/util/atomic_value64_offset.h \
	src/util/atomic_value.h \
	src/util/barrier.h \
	src/util/bitmap.h \
	src/util/malloc.h \
        src/util/malloc.cc \
        src/util/operation_logger.h \
        src/util/platform.h \
        src/util/random.h \
        src/util/random.cc \
        src/util/threadlocals.h \
        src/util/threadlocals.cc \
	src/util/time.h \
	src/util/workloads.h \
	src/util/workloads.cc

#
# BFS graph benchmark
#

bin_PROGRAMS += bfs-analyzer
bfs_analyzer_SOURCES = \
	$(UTIL_OBJS) \
        src/benchmark/bfs/bfs_analyzer.cc \
	src/benchmark/bfs/graph.h \
        src/benchmark/bfs/graph.cc

bin_PROGRAMS += bfs-sequential
bfs_sequential_SOURCES = \
	$(UTIL_OBJS) \
        src/benchmark/bfs/bfs_sequential.cc \
	src/benchmark/bfs/graph.h \
        src/benchmark/bfs/graph.cc

#
# Shortest Path benchmark
#

SHORTESTPATH_BASE_OBJS = \
	$(UTIL_OBJS) \
        src/benchmark/common.cc \
        src/benchmark/shortest_path/shortest_path.cc

bin_PROGRAMS += shortest-path-bskfifo
shortest_path_bskfifo_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_bskfifo.cc

bin_PROGRAMS += shortest-path-fc
shortest_path_fc_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_fc_queue.cc

bin_PROGRAMS += shortest-path-lb
shortest_path_lb_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_lb_queue.cc

bin_PROGRAMS += shortest-path-kstack
shortest_path_kstack_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_kstack.cc

bin_PROGRAMS += shortest-path-ms
shortest_path_ms_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_ms_queue.cc

bin_PROGRAMS += shortest-path-sq
shortest_path_sq_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_s_queue.cc

bin_PROGRAMS += shortest-path-ah
shortest_path_ah_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_ah_queue.cc

bin_PROGRAMS += shortest-path-enq
shortest_path_enq_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_enq_pool.cc

bin_PROGRAMS += shortest-path-dq-1random
shortest_path_dq_1random_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random.cc

bin_PROGRAMS += shortest-path-dq-2random
shortest_path_dq_2random_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_2random.cc

bin_PROGRAMS += shortest-path-dq-1random-tstack
shortest_path_dq_1random_tstack_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random_tstack.cc

bin_PROGRAMS += shortest-path-dq-partrr
shortest_path_dq_partrr_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_partrr.cc

bin_PROGRAMS += shortest-path-rd
shortest_path_rd_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_rd_queue.cc

bin_PROGRAMS += shortest-path-tstack
shortest_path_tstack_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_treiber_stack.cc

bin_PROGRAMS += shortest-path-uskfifo
shortest_path_uskfifo_SOURCES = \
        $(SHORTESTPATH_BASE_OBJS) \
        src/benchmark/std_glue/glue_uskfifo.cc

bin_PROGRAMS += shortest-path-wf-ppopp11
shortest_path_wf_ppopp11_SOURCES = \
				$(SHORTESTPATH_BASE_OBJS) \
				src/benchmark/std_glue/glue_wf_ppopp11.cc

bin_PROGRAMS += shortest-path-wf-ppopp12
shortest_path_wf_ppopp12_SOURCES = \
				$(SHORTESTPATH_BASE_OBJS) \
				src/benchmark/std_glue/glue_wf_ppopp12.cc

#
# Producer/Consumer benchmark
#

SEQALT_BASE_OBJS = \
	$(UTIL_OBJS) \
        src/benchmark/common.cc \
        src/benchmark/seqalt/seqalt.cc

bin_PROGRAMS += seqalt-bskfifo
seqalt_bskfifo_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_bskfifo.cc

bin_PROGRAMS += seqalt-fc
seqalt_fc_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_fc_queue.cc

bin_PROGRAMS += seqalt-ah
seqalt_ah_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_ah_queue.cc

bin_PROGRAMS += seqalt-enq
seqalt_enq_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_enq_pool.cc

bin_PROGRAMS += seqalt-lb
seqalt_lb_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_lb_queue.cc

bin_PROGRAMS += seqalt-kstack
seqalt_kstack_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_kstack.cc

bin_PROGRAMS += seqalt-ms
seqalt_ms_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_ms_queue.cc

bin_PROGRAMS += seqalt-sq
seqalt_sq_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_s_queue.cc

bin_PROGRAMS += seqalt-dq-1random
seqalt_dq_1random_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random.cc

bin_PROGRAMS += seqalt-dq-2random
seqalt_dq_2random_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_2random.cc

bin_PROGRAMS += seqalt-dq-1random-tstack
seqalt_dq_1random_tstack_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random_tstack.cc

bin_PROGRAMS += seqalt-dq-partrr
seqalt_dq_partrr_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_partrr.cc

bin_PROGRAMS += seqalt-rd
seqalt_rd_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_rd_queue.cc

bin_PROGRAMS += seqalt-tstack
seqalt_tstack_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_treiber_stack.cc

bin_PROGRAMS += seqalt-uskfifo
seqalt_uskfifo_SOURCES = \
        $(SEQALT_BASE_OBJS) \
        src/benchmark/std_glue/glue_uskfifo.cc

bin_PROGRAMS += seqalt-wf-ppopp11
seqalt_wf_ppopp11_SOURCES = \
	$(SEQALT_BASE_OBJS) \
	src/benchmark/std_glue/glue_wf_ppopp11.cc

bin_PROGRAMS += seqalt-wf-ppopp12
seqalt_wf_ppopp12_SOURCES = \
	$(SEQALT_BASE_OBJS) \
	src/benchmark/std_glue/glue_wf_ppopp12.cc

#
# Producer/Consumer benchmark
#

PRODCON_BASE_OBJS = \
	$(UTIL_OBJS) \
        src/benchmark/common.cc \
        src/benchmark/prodcon/prodcon.cc

bin_PROGRAMS += prodcon-bskfifo
prodcon_bskfifo_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_bskfifo.cc

bin_PROGRAMS += prodcon-fc
prodcon_fc_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_fc_queue.cc

bin_PROGRAMS += prodcon-ah
prodcon_ah_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_ah_queue.cc

bin_PROGRAMS += prodcon-enq
prodcon_enq_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_enq_pool.cc

bin_PROGRAMS += prodcon-lb
prodcon_lb_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_lb_queue.cc

bin_PROGRAMS += prodcon-kstack
prodcon_kstack_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_kstack.cc

bin_PROGRAMS += prodcon-ms
prodcon_ms_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_ms_queue.cc

bin_PROGRAMS += prodcon-sq
prodcon_sq_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_s_queue.cc

bin_PROGRAMS += prodcon-dq-1random
prodcon_dq_1random_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random.cc

bin_PROGRAMS += prodcon-dq-2random
prodcon_dq_2random_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_2random.cc

bin_PROGRAMS += prodcon-dq-1random-tstack
prodcon_dq_1random_tstack_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random_tstack.cc

bin_PROGRAMS += prodcon-dq-partrr
prodcon_dq_partrr_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_partrr.cc

bin_PROGRAMS += prodcon-rd
prodcon_rd_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_rd_queue.cc

bin_PROGRAMS += prodcon-tstack
prodcon_tstack_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_treiber_stack.cc

bin_PROGRAMS += prodcon-uskfifo
prodcon_uskfifo_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_uskfifo.cc

bin_PROGRAMS += prodcon-wf-ppopp11
prodcon_wf_ppopp11_SOURCES = \
	$(PRODCON_BASE_OBJS) \
	src/benchmark/std_glue/glue_wf_ppopp11.cc

bin_PROGRAMS += prodcon-wf-ppopp12
prodcon_wf_ppopp12_SOURCES = \
	$(PRODCON_BASE_OBJS) \
	src/benchmark/std_glue/glue_wf_ppopp12.cc

#
# SPF benchmark
#

bin_PROGRAMS += spf-sequential
spf_sequential_SOURCES = \
	$(UTIL_OBJS) \
        src/benchmark/bfs/spf_sequential.cc \
	src/benchmark/bfs/graph.h \
        src/benchmark/bfs/graph.cc

#
# Tests -- currenctly only the atomic containers
#

gtest:
	$(AM_V_GEN)mkdir -p gtest
	cd gtest ; cmake /usr/src/gtest
	$(AM_V_GEN)make -C gtest all

gtest/libgtest_main.a: gtest

gtest/libgtest.a: gtest

gtest-clean:
	$(AM_V_GEN)rm -rf gtest

GTEST_LIBS = gtest/libgtest.a gtest/libgtest_main.a

TEST_CPPFLAGS = -Werror -mcx16 -m64 -std=c++0x \
	@CC_OPTIMIZATION@ \
        -I${top_srcdir}/src \
        -ffast-math \
        -fno-builtin-malloc \
        -fno-builtin-calloc \
        -fno-builtin-realloc \
        -fno-builtin-free \
        -fno-omit-frame-pointer \
	@GFLAGS_CFLAGS@

TESTS += atomic_value128_unittest
atomic_value128_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS)
atomic_value128_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
atomic_value128_unittest_SOURCES = \
        src/test/atomic_value128_unittest.cc \
        src/util/malloc.cc

TESTS += atomic_value64_no_offset_unittest
atomic_value64_no_offset_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS)
atomic_value64_no_offset_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
atomic_value64_no_offset_unittest_SOURCES = \
        src/test/atomic_value64_no_offset_unittest.cc \
        src/util/malloc.cc

TESTS += atomic_value64_offset_unittest
atomic_value64_offset_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS)
atomic_value64_offset_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
atomic_value64_offset_unittest_SOURCES = \
        src/test/atomic_value64_offset_unittest.cc \
        src/util/malloc.cc

TESTS += random_unittest
random_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS)
random_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
random_unittest_SOURCES = \
        src/test/random_unittest.cc \
        src/util/random.cc \
        src/util/threadlocals.cc

noinst_PROGRAMS += $(TESTS)

#
# Extend the standard Makefile rules
#

clean-local: gtest-clean

