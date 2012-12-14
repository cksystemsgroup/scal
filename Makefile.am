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
        -fno-omit-frame-pointer

AM_LDFLAGS = -Wl,--as-needed 
LDADD = -lm @GFLAGS_LIBS@ -lrt

# tcmalloc stuff
LDADD += external/libtcmalloc_minimal.a

#
# Mandelbrot calculation benchmark
#

#MANDEL_BASE_OBJS = \
#        src/benchmark/common.cc \
#        src/benchmark/mandelbrot/mandel.cc \
#        src/benchmark/mandelbrot/bitmap.c \
#        src/util/histogram.cc \
#        src/util/malloc.cc \
#        src/util/operation_logger.cc \
#        src/util/profiler.cc \
#        src/util/random.cc \
#        src/util/threadlocals.cc 
#
#bin_PROGRAMS += mandel-bskfifo
#mandel_bskfifo_SOURCES = \
#        $(MANDEL_BASE_OBJS) \
#        src/benchmark/std_glue/glue_bskfifo.cc
#
#bin_PROGRAMS += mandel-ms
#mandel_ms_SOURCES = \
#        $(MANDEL_BASE_OBJS) \
#        src/benchmark/std_glue/glue_ms_queue.cc
#
#bin_PROGRAMS += mandel-scal1random
#mandel_scal1random_SOURCES = \
#        $(MANDEL_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_1random.cc
#
#bin_PROGRAMS += mandel-scalpartrr
#mandel_scalpartrr_SOURCES = \
#        $(MANDEL_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_partrr.cc
#
#bin_PROGRAMS += mandel-uskfifo
#mandel_uskfifo_SOURCES = \
#        $(MANDEL_BASE_OBJS) \
#        src/benchmark/std_glue/glue_uskfifo.cc

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

#bin_PROGRAMS += prodcon-bskfifo
#prodcon_bskfifo_SOURCES = \
#        $(PRODCON_BASE_OBJS) \
#        src/benchmark/std_glue/glue_bskfifo.cc

bin_PROGRAMS += prodcon-ms
prodcon_ms_SOURCES = \
        $(PRODCON_BASE_OBJS) \
        src/benchmark/std_glue/glue_ms_queue.cc

#bin_PROGRAMS += prodcon-scal1random
#prodcon_scal1random_SOURCES = \
#        $(PRODCON_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_1random.cc

#bin_PROGRAMS += prodcon-scalpartrr
#prodcon_scalpartrr_SOURCES = \
#        $(PRODCON_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_partrr.cc

#bin_PROGRAMS += prodcon-uskfifo
#prodcon_uskfifo_SOURCES = \
#        $(PRODCON_BASE_OBJS) \
#        src/benchmark/std_glue/glue_uskfifo.cc

#
# Sequential alternating benchmark
#

#SEQALT_BASE_OBJS = \
#        src/benchmark/common.cc \
#        src/benchmark/seqalt/seqalt-old.cc \
#        src/util/histogram.cc \
#        src/util/history.cc \
#        src/util/malloc.cc \
#        src/util/operation_logger.cc \
#        src/util/profiler.cc \
#        src/util/random.cc \
#        src/util/threadlocals.cc \
#        src/util/workloads.cc 
#
#bin_PROGRAMS += seqalt-bskfifo
#seqalt_bskfifo_SOURCES = \
#        $(SEQALT_BASE_OBJS) \
#        src/benchmark/std_glue/glue_bskfifo.cc
#
#bin_PROGRAMS += seqalt-ms
#seqalt_ms_SOURCES = \
#        $(SEQALT_BASE_OBJS) \
#        src/benchmark/std_glue/glue_ms_queue.cc
#
#bin_PROGRAMS += seqalt-scal1random
#seqalt_scal1random_SOURCES = \
#        $(SEQALT_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_1random.cc
#
#bin_PROGRAMS += seqalt-scalpartrr
#seqalt_scalpartrr_SOURCES = \
#        $(SEQALT_BASE_OBJS) \
#        src/benchmark/std_glue/glue_scal_partrr.cc
#
#bin_PROGRAMS += seqalt-uskfifo
#seqalt_uskfifo_SOURCES = \
#        $(SEQALT_BASE_OBJS) \
#        src/benchmark/std_glue/glue_uskfifo.cc

#
# Tests -- currenctly only the atomic containers
#

TESTS += atomic_value128_unittest
atomic_value128_unittest_CPPFLAGS = \
        -Wall -mcx16 -m64  -std=c++0x \
        -I./src \
        -I./external/gtest-1.6.0/include
atomic_value128_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        external/gtest_main.a 
atomic_value128_unittest_SOURCES = \
        src/test/atomic_value128_unittest.cc \
        src/util/malloc.cc

TESTS += atomic_value64_unittest
atomic_value64_unittest_CPPFLAGS = \
        -Wall -m64  -std=c++0x \
        -I./src \
        -I./external/gtest-1.6.0/include
atomic_value64_unittest_LDADD = \
        @GFLAGS_LIBS@ \
        external/gtest_main.a 
atomic_value64_unittest_SOURCES = \
        src/test/atomic_value64_unittest.cc \
        src/util/malloc.cc

noinst_PROGRAMS += $(TESTS)
