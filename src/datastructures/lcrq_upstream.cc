// Copyright (c) 2013, Adam Morrison and Yehuda Afek.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of the Tel Aviv University nor the names of the
//    author of this software may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>

// system.h
#ifndef CACHE_LINE_SIZE
#    define CACHE_LINE_SIZE            64
#endif

#ifdef __GNUC__
#    define CACHE_ALIGN                __attribute__ ((aligned (CACHE_LINE_SIZE)))
#    define VAR_ALIGN                  __attribute__ ((aligned (16)))
#elif defined(MSVC)
#    define CACHE_ALIGN                __declspec(align(CACHE_LINE_SIZE)) 
#    define VAR_ALIGN                  __declspec(align(16)) 
#else
#    define CACHE_ALIGN
#endif


#define PAD_CACHE(A)                  ((CACHE_LINE_SIZE - (A % CACHE_LINE_SIZE))/sizeof(int32_t))


#ifndef USE_CPUS
#    if defined(linux)
#        define USE_CPUS               sysconf(_SC_NPROCESSORS_ONLN)
#    else
#        define USE_CPUS               1
#    endif
#endif
// end system.h

// types.h
typedef union int_aligned32_t {
   int32_t v CACHE_ALIGN;
   char pad[CACHE_LINE_SIZE];
}  int_aligned32_t;

typedef union int_aligned64_t {
   int64_t v CACHE_ALIGN;
   char pad[CACHE_LINE_SIZE];
}  int_aligned64_t;

#define null                           NULL
#define bool                           int32_t
#define true                           1
#define false                          0
// end types.h


// rand.h
#include <math.h>
#define SIM_RAND_MAX         32767

// This random generators are implementing 
// by following POSIX.1-2001 directives.
// ---------------------------------------

__thread unsigned long next = 1;

inline static long simRandom(void) {
    next = next * 1103515245 + 12345;
    return((unsigned)(next/65536) % 32768);
}

inline static void simSRandom(unsigned long seed) {
    next = seed;
}

// In Numerical Recipes in C: The Art of Scientific Computing 
// (William H. Press, Brian P. Flannery, Saul A. Teukolsky, William T. Vetterling;
// New York: Cambridge University Press, 1992 (2nd ed., p. 277))
// -------------------------------------------------------------------------------

inline static long simRandomRange(long low, long high) {
    return low + (long) ( ((double) high)* (simRandom() / (SIM_RAND_MAX + 1.0)));
}
// end rand.h

typedef long Object;
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)



// primitives .h
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <math.h>
#include <stdint.h>
#include <sys/timeb.h>
#include <malloc.h>

//#include "stats.h"
//#include "types.h"
//#include "system.h"

//#define _EMULATE_FAA_
//#define _EMULATE_SWAP_

inline static void *getMemory(size_t size) {
    void *p = malloc(size);

    if (p == null) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    } else return p;
}

inline static void *getAlignedMemory(size_t align, size_t size) {
    void *p;
    
    p = (void *)memalign(align, size);
    if (p == null) {
        perror("memalign");
        exit(EXIT_FAILURE);
    } else return p;
}

inline static int64_t getTimeMillis(void) {
    struct timeb tm;

    ftime(&tm);
    return 1000 * tm.time + tm.millitm;
}


#if defined(__GNUC__) && (__GNUC__*10000 + __GNUC_MINOR__*100) >= 40100
#    define __CASPTR(A, B, C)          __sync_bool_compare_and_swap((long *)A, (long)B, (long)C)
#    define __CAS64(A, B, C)           __sync_bool_compare_and_swap(A, B, C)
#    define __CAS32(A, B, C)           __sync_bool_compare_and_swap(A, B, C)
#    define __SWAP(A, B)               __sync_lock_test_and_set((long *)A, (long)B)
#    define __FAA64(A, B)              __sync_fetch_and_add(A, B)
#    define __FAA32(A, B)              __sync_fetch_and_add(A, B)
#    define ReadPrefetch(A)            __builtin_prefetch((const void *)A, 0, 3);
#    define StorePrefetch(A)           __builtin_prefetch((const void *)A, 1, 3);
#    define bitSearchFirst(A)          __builtin_ctzll(A)
#    define nonZeroBits(A)             __builtin_popcountll(A)
#    if defined(__amd64__) || defined(__x86_64__)
#        define LoadFence()            asm volatile ("lfence":::"memory")
#        define StoreFence()           asm volatile ("sfence":::"memory")
#        define FullFence()            asm volatile ("mfence":::"memory")
#    else
#        define LoadFence()            __sync_synchronize()
#        define StoreFence()           __sync_synchronize()
#        define FullFence()            __sync_synchronize()
#    endif

#elif defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__))
#    warning A newer GCC version is recommended!
#    define LoadFence()                asm volatile ("lfence":::"memory") 
#    define StoreFence()               asm volatile ("sfence":::"memory") 
#    define FullFence()                asm volatile ("mfence":::"memory") 
#    define ReadPrefetch(A)            asm volatile ("prefetch0 %0"::"m"((const void *)A))
#    define StorePrefetch(A)           asm volatile ("prefetch0 %0"::"m"((const void *)A))

inline static bool __CASPTR(void *A, void *B, void *C) {
    uint64_t prev;
    uint64_t *p = (uint64_t *)A;

    asm volatile("lock;cmpxchgq %1,%2"
         : "=a"(prev)
         : "r"((uint64_t)C), "m"(*p), "0"((uint64_t)B)
         : "memory");
    return (prev == (uint64_t)B);
}

inline static bool __CAS64(uint64_t *A, uint64_t B, uint64_t C) {
    uint64_t prev;
    uint64_t *p = (int64_t *)A;

    asm volatile("lock;cmpxchgq %1,%2"
         : "=a"(prev)
         : "r"(C), "m"(*p), "0"(B)
         : "memory");
    return (prev == B);
}

inline static bool __CAS32(uint32_t *A, uint32_t B, uint32_t C) {
    uint32_t prev;
    uint32_t *p = (uint32_t *)A;

    asm volatile("lock;cmpxchgl %1,%2"
         : "=a"(prev)
         : "r"(C), "m"(*p), "0"(B)
         : "memory");
    return (prev == B);
}

inline static void *__SWAP(void *A, void *B) {
    int64_t *p = (int64_t *)A;

    asm volatile("lock;"
          "xchgq %0, %1"
          : "=r"(B), "=m"(*p)
          : "0"(B), "m"(*p)
          : "memory");
    return B;
}

inline static uint64_t __FAA64(volatile uint64_t *A, uint64_t B){
    asm("lock; xaddq %0,%1": "+r" (B), "+m" (*(A)): : "memory");
}

inline static uint32_t __FAA32(volatile uint32_t *A, uint32_t B){
    asm("lock; xaddl %0,%1": "+r" (B), "+m" (*(A)): : "memory");
}

inline static int bitSearchFirst(uint64_t B) {
    uint64_t A;

    asm("bsfq %0, %1;" : "=d"(A) : "d"(B));

    return (int)A;
}

inline static uint64_t nonZeroBits(uint64_t v) {
    uint64_t c;

    for (c = 0; v; v >>= 1)
        c += v & 1;

    return c;
}

#elif defined(sun) && defined(sparc) && defined(__SUNPRO_C)
#    warning Experimental support!

#    include <atomic.h>
#    include <sun_prefetch.h>

     extern void MEMBAR_ALL(void);
     extern void MEMBAR_STORE(void);
     extern void MEMBAR_LOAD(void);
     extern void *CASPO(void volatile*, void *);
     extern void *SWAPPO(void volatile*, void *);
     extern int32_t POPC(int32_t x);
     extern void NOP(void);

#    define __CASPTR(A, B, C)          (atomic_cas_ptr(A, B, C) == B)
#    define __CAS32(A, B, C)           (atomic_cas_32(A, B, C) == B)
#    define __CAS64(A, B, C)           (atomic_cas_64(A, B, C) == B)
#    define __SWAP(A, B)               SWAPPO(A, B)
#    define __FAA32(A, B)              atomic_add_32_nv(A, B)
#    define __FAA64(A, B)              atomic_add_64_nv(A, B)
#    define nonZeroBits(A)             (POPC((int32_t)A)+POPC((int32_t)(A>>32)))
#    define LoadFence()                MEMBAR_LOAD()
#    define StoreFence()               MEMBAR_STORE()
#    define FullFence()                MEMBAR_ALL()
#    define ReadPrefetch(A)            sparc_prefetch_read_many((void *)A)
#    define StorePrefetch(A)           sparc_prefetch_write_many((void *)A)

inline static uint32_t __bitSearchFirst32(uint32_t v) {
    register uint32_t r;     // The result of log2(v) will be stored here
    register uint32_t shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);
    return r;
}

inline static uint32_t bitSearchFirst(uint64_t v) {
    uint32_t r = __bitSearchFirst32((uint32_t)v);
    return (r == 0) ? __bitSearchFirst32((uint32_t)(v >> 32)) + 31 : r;
}

#else
#    error Current machine architecture and compiler are not supported yet!
#endif

#define CAS32(A, B, C) _CAS32((uint32_t *)A, (uint32_t)B, (uint32_t)C)
inline static bool _CAS32(uint32_t *A, uint32_t B, uint32_t C) {
#ifdef DEBUG
    int res;

    res = __CAS32(A, B, C);
    __executed_cas[__stats_thread_id].v++;
    __failed_cas[__stats_thread_id].v += 1 - res;
    
    return res;
#else
    return __CAS32(A, B, C);
#endif
}

#define CAS64(A, B, C) _CAS64((uint64_t *)A, (uint64_t)B, (uint64_t)C)
inline static bool _CAS64(uint64_t *A, uint64_t B, uint64_t C) {
#ifdef DEBUG
    int res;

    res = __CAS64(A, B, C);
    __executed_cas[__stats_thread_id].v++;
    __failed_cas[__stats_thread_id].v += 1 - res;
    
    return res;
#else
    return __CAS64(A, B, C);
#endif
}

#define CASPTR(A, B, C) _CASPTR((void *)A, (void *)B, (void *)C)
inline static bool _CASPTR(void *A, void *B, void *C) {
#ifdef DEBUG
    int res;

    res = __CASPTR(A, B, C);
    __executed_cas[__stats_thread_id].v++;
    __failed_cas[__stats_thread_id].v += 1 - res;
    
    return res;
#else
    return __CASPTR(A, B, C);
#endif
}

#define SWAP(A, B) _SWAP((void *)A, (void *)B)
inline static void *_SWAP(void *A, void *B) {
#if defined(_EMULATE_SWAP_)
#    warning SWAP instructions are simulated!
    void *old_val;
    void *new_val;

    while (true) {
        old_val = (void *)*((volatile long *)A);
        new_val = B;
        if(((void *)*((volatile long *)A)) == old_val && CASPTR(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
    __executed_swap[__stats_thread_id].v++;
#   endif
    return old_val;

#else
#ifdef DEBUG
    __executed_swap[__stats_thread_id].v++;
    return (void *)__SWAP(A, B);
#else
    return (void *)__SWAP(A, B);
#endif
#endif
}

#define FAA32(A, B) _FAA32((volatile uint32_t *)A, (uint32_t)B)
inline static uint32_t _FAA32(volatile uint32_t *A, uint32_t B) {
#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int32_t old_val;
    int32_t new_val;

    while (true) {
        old_val = *((int32_t * volatile)A);
        new_val = old_val + B;
        if(*A == old_val && CAS32(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
        __executed_faa[__stats_thread_id].v++;
#   endif
    return old_val;

#else
#   ifdef DEBUG
    __executed_faa[__stats_thread_id].v++;
    return __FAA32(A, B);
#   else
    return __FAA32(A, B);
#   endif
#endif
}

#define FAA64(A, B) _FAA64((volatile int64_t *)A, (int64_t)B)
inline static int64_t _FAA64(volatile int64_t *A, int64_t B) {

#if defined(_EMULATE_FAA_)
#    warning Fetch&Add instructions are simulated!

    int64_t old_val;
    int64_t new_val;

    while (true) {
        old_val = *((int64_t * volatile)A);
        new_val = old_val + B;
        if(*A == old_val && CAS64(A, old_val, new_val) == true)
            break;
    }
#   ifdef DEBUG
        __executed_faa[__stats_thread_id].v++;
#   endif
    return old_val;

#else
#   ifdef DEBUG
    __executed_faa[__stats_thread_id].v++;
    return __FAA64(A, B);
#   else
    return __FAA64(A, B);
#   endif
#endif
}
// end primitives.h


// Definition: RING_POW
// --------------------
// The LCRQ's ring size will be 2^{RING_POW}.
#ifndef RING_POW
#define RING_POW        (17)
#endif
#define RING_SIZE       (1ull << RING_POW)

// Definition: RING_STATS
// --------------------
// Define to collect statistics about CRQ closes and nodes
// marked unsafe.
//#define RING_STATS

// Definition: HAVE_HPTRS
// --------------------
// Define to enable hazard pointer setting for safe memory
// reclamation.  You'll need to integrate this with your
// hazard pointers implementation.
//#define HAVE_HPTRS

#define __CAS2(ptr, o1, o2, n1, n2)                             \
({                                                              \
    char __ret;                                                 \
    __typeof__(o2) __junk;                                      \
    __typeof__(*(ptr)) __old1 = (o1);                           \
    __typeof__(o2) __old2 = (o2);                               \
    __typeof__(*(ptr)) __new1 = (n1);                           \
    __typeof__(o2) __new2 = (n2);                               \
    asm volatile("lock cmpxchg16b %2;setz %1"                   \
                   : "=d"(__junk), "=a"(__ret), "+m" (*ptr)     \
                   : "b"(__new1), "c"(__new2),                  \
                     "a"(__old1), "d"(__old2));                 \
    __ret; })

#ifdef DEBUG
#define CAS2(ptr, o1, o2, n1, n2)                               \
({                                                              \
    int res;                                                    \
    res = __CAS2(ptr, o1, o2, n1, n2);                          \
    __executed_cas[__stats_thread_id].v++;                      \
    __failed_cas[__stats_thread_id].v += 1 - res;               \
    res;                                                        \
})
#else
#define CAS2(ptr, o1, o2, n1, n2)    __CAS2(ptr, o1, o2, n1, n2)
#endif


#define BIT_TEST_AND_SET(ptr, b)                                \
({                                                              \
    char __ret;                                                 \
    asm volatile("lock btsq $63, %0; setnc %1" : "+m"(*ptr), "=a"(__ret) : : "cc"); \
    __ret;                                                      \
})


inline int is_empty(uint64_t v) __attribute__ ((pure));
inline uint64_t node_index(uint64_t i) __attribute__ ((pure));
inline uint64_t set_unsafe(uint64_t i) __attribute__ ((pure));
inline uint64_t node_unsafe(uint64_t i) __attribute__ ((pure));
inline uint64_t tail_index(uint64_t t) __attribute__ ((pure));
inline int crq_is_closed(uint64_t t) __attribute__ ((pure));

typedef struct RingNode {
    volatile uint64_t val;
    volatile uint64_t idx;
    uint64_t pad[14];
} RingNode __attribute__ ((aligned (128)));

typedef struct RingQueue {
    volatile int64_t head __attribute__ ((aligned (128)));
    volatile int64_t tail __attribute__ ((aligned (128)));
    struct RingQueue *next __attribute__ ((aligned (128)));
    RingNode array[RING_SIZE];
} RingQueue __attribute__ ((aligned (128)));

RingQueue *head;
RingQueue *tail;

inline void init_ring(RingQueue *r) {
    int i;

    for (i = 0; i < RING_SIZE; i++) {
        r->array[i].val = -1;
        r->array[i].idx = i;
    }

    r->head = r->tail = 0;
    r->next = null;
}

int FULL;


inline int is_empty(uint64_t v)  {
    return (v == (uint64_t)-1);
}


inline uint64_t node_index(uint64_t i) {
    return (i & ~(1ull << 63));
}


inline uint64_t set_unsafe(uint64_t i) {
    return (i | (1ull << 63));
}


inline uint64_t node_unsafe(uint64_t i) {
    return (i & (1ull << 63));
}


inline uint64_t tail_index(uint64_t t) {
    return (t & ~(1ull << 63));
}


inline int crq_is_closed(uint64_t t) {
    return (t & (1ull << 63)) != 0;
}


void lcrq_SHARED_OBJECT_INIT() {
     int i;

     RingQueue *rq = reinterpret_cast<RingQueue*>(getMemory(sizeof(RingQueue)));
     init_ring(rq);
     head = tail = rq;

     if (FULL) {
         // fill ring 
         for (i = 0; i < RING_SIZE/2; i++) {
             rq->array[i].val = 0;
             rq->array[i].idx = i;
             rq->tail++;
         }
         FULL = 0;
    }
}


inline void fixState(RingQueue *rq) {

    uint64_t t, h, n;

    while (1) {
        uint64_t t = FAA64(&rq->tail, 0);
        uint64_t h = FAA64(&rq->head, 0);

        if (unlikely(rq->tail != t))
            continue;

        if (h > t) {
            if (CAS64(&rq->tail, t, h)) break;
            continue;
        }
        break;
    }
}

__thread RingQueue *nrq;
__thread RingQueue *hazardptr;

/*
#ifdef RING_STATS
__thread uint64_t mycloses;
__thread uint64_t myunsafes;

uint64_t closes;
uint64_t unsafes;

inline void count_closed_crq(void) {
    mycloses++;
}


inline void count_unsafe_node(void) {
    myunsafes++;
}
#else
inline void count_closed_crq(void) { }
inline void count_unsafe_node(void) { }
#endif
*/


inline int close_crq(RingQueue *rq, const uint64_t t, const int tries) {
    if (tries < 10)
        return CAS64(&rq->tail, t + 1, (t + 1)|(1ull<<63));
    else
        return BIT_TEST_AND_SET(&rq->tail, 63);
}

void lcrq_enqueue(Object arg) {

    int try_close = 0;

    while (1) {
        RingQueue *rq = tail;

#ifdef HAVE_HPTRS
        SWAP(&hazardptr, rq);
        if (unlikely(tail != rq))
            continue;
#endif

        RingQueue *next = rq->next;
 
        if (unlikely(next != null)) {
            CASPTR(&tail, rq, next);
            continue;
        }

        uint64_t t = FAA64(&rq->tail, 1);

        if (crq_is_closed(t)) {
alloc:
            if (nrq == null) {
                nrq = reinterpret_cast<RingQueue*>(getMemory(sizeof(RingQueue)));
                init_ring(nrq);
            }

            // Solo enqueue
            nrq->tail = 1, nrq->array[0].val = arg, nrq->array[0].idx = 0;

            if (CASPTR(&rq->next, null, nrq)) {
                CASPTR(&tail, rq, nrq);
                nrq = null;
                return;
            }
            continue;
        }
           
        RingNode* cell = &rq->array[t & (RING_SIZE-1)];
        StorePrefetch(cell);

        uint64_t idx = cell->idx;
        uint64_t val = cell->val;

        if (likely(is_empty(val))) {
            if (likely(node_index(idx) <= t)) {
                if ((likely(!node_unsafe(idx)) || rq->head < t) && CAS2((uint64_t*)cell, -1, idx, arg, t)) {
                    return;
                }
            }
        } 

        uint64_t h = rq->head;

        if (unlikely(t - h >= RING_SIZE) && close_crq(rq, t, ++try_close)) {
            goto alloc;
        }
    }
}

Object lcrq_dequeue() {

    while (1) {
        RingQueue *rq = head;
        RingQueue *next;

#ifdef HAVE_HPTRS
        SWAP(&hazardptr, rq);
        if (unlikely(head != rq))
            continue;
#endif

        uint64_t h = FAA64(&rq->head, 1);


        RingNode* cell = &rq->array[h & (RING_SIZE-1)];
        StorePrefetch(cell);

        uint64_t tt;
        int r = 0;

        while (1) {

            uint64_t cell_idx = cell->idx;
            uint64_t unsafe = node_unsafe(cell_idx);
            uint64_t idx = node_index(cell_idx);
            uint64_t val = cell->val;

            if (unlikely(idx > h)) break;

            if (likely(!is_empty(val))) {
                if (likely(idx == h)) {
                    if (CAS2((uint64_t*)cell, val, cell_idx, -1, unsafe | h + RING_SIZE))
                        return val;
                } else {
                    if (CAS2((uint64_t*)cell, val, cell_idx, val, set_unsafe(idx))) {
                        break;
                    }
                }
            } else {
                if ((r & ((1ull << 10) - 1)) == 0)
                    tt = rq->tail;

                // Optimization: try to bail quickly if queue is closed.
                int crq_closed = crq_is_closed(tt);
                uint64_t t = tail_index(tt);

                if (unlikely(unsafe)) { // Nothing to do, move along
                    if (CAS2((uint64_t*)cell, val, cell_idx, val, unsafe | h + RING_SIZE))
                        break;
                } else if (t - 1 <= h || r > 200000 || crq_closed) {
                    if (CAS2((uint64_t*)cell, val, idx, val, h + RING_SIZE))
                        break;
                } else {
                    ++r;
                }
            }
        }

        if (tail_index(rq->tail) - 1 <= h) {
            fixState(rq);
            // try to return empty
            next = rq->next;
            if (next == null)
                return -1;  // EMPTY
            CASPTR(&head, rq, next);
        }
    }
}
