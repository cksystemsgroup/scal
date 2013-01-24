# We'll add the programs throughout the Makefile.
bin_PROGRAMS =
noinst_PROGRAMS =
TESTS = 

AM_CPPFLAGS = -Wall -mcx16 -m64  -std=c++0x \
        @CC_OPTIMIZATION@ \
        -I./src \
        -ffast-math \
        -fno-builtin-malloc \
        -fno-builtin-calloc \
        -fno-builtin-realloc \
        -fno-builtin-free \
        -fno-omit-frame-pointer \
	@GFLAGS_CFLAGS@

AM_LDFLAGS = -Wl,--as-needed 
LDADD = @GFLAGS_LIBS@

GTEST_DIR = external/googletest

#
# Producer/Consumer benchmark
#

PRODCON_BASE_OBJS = \
        src/benchmark/common.cc \
        src/benchmark/prodcon/prodcon.cc \
        src/util/malloc.cc \
        src/util/random.cc \
        src/util/threadlocals.cc \
        src/util/workloads.cc

bin_PROGRAMS += prodcon-bskfifo
prodcon_bskfifo_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_bskfifo.cc

bin_PROGRAMS += prodcon-fc
prodcon_fc_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_fc_queue.cc

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

bin_PROGRAMS += prodcon-dq-1random
prodcon_dq_1random_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_dq_1random.cc

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
# Tests -- currenctly only the atomic containers
#

external/libgtest_main.a:
	$(CXX) -I$(GTEST_DIR)/include -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest_main.cc -o external/gtest_main.o
	$(AR) -rv external/libgtest_main.a external/gtest_main.o

external/libgtest.a:
	$(CXX) -I$(GTEST_DIR)/include -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest-all.cc -o external/gtest-all.o
	$(AR) -rv external/libgtest.a external/gtest-all.o

gtest-clean:
	rm -f external/gtest-all.o external/gtest_main.o
	rm -f external/libgtest.a external/libgtest_main.a

GTEST_LIBS = external/libgtest.a external/libgtest_main.a

TEST_CPPFLAGS = -Wall -mcx16 -m64 -std=c++0x

TESTS += atomic_value128_unittest
atomic_value128_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS) \
        -I./src \
        -I$(GTEST_DIR)/include
atomic_value128_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
atomic_value128_unittest_SOURCES = \
        src/test/atomic_value128_unittest.cc \
        src/util/malloc.cc

TESTS += atomic_value64_unittest
atomic_value64_unittest_CPPFLAGS = \
	$(TEST_CPPFLAGS) \
        -I./src \
        -I$(GTEST_DIR)/include
atomic_value64_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        $(GTEST_LIBS) 
atomic_value64_unittest_SOURCES = \
        src/test/atomic_value64_unittest.cc \
        src/util/malloc.cc

noinst_PROGRAMS += $(TESTS)

#
# Extend the standard Makefile rules
#

clean-local: gtest-clean

