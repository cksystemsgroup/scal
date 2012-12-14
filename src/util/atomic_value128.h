// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ATOMIC_VALUE128_H_
#define SCAL_UTIL_ATOMIC_VALUE128_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/malloc.h"

// Invariants for working with AtomicValue128:
//
// * Memory is always consistent when reading using .raw(), .aba(),
//   .value(), and copy ctor.
// * Memory is consistent if objects are written through .set_raw(),
//   .set_aba(), and .set_value().
//
// As a consequence (for the lazy):
//
// * Memory may be inconsistent when using assignment operator in a
//   concurrent workload.
// 
// AtomicValue128 internal layout:
// 64bit value | 64 bt ABA
// |high--------------low|

typedef unsigned int uint128_t __attribute__((mode(TI)));

#define UINT128_C(X) (0u + ((uint128_t)(X)))

typedef uint64_t AtomicAba;
typedef uint128_t AtomicRaw;

template<typename T>
class AtomicValue128 {
 public:
  static AtomicValue128<T>* get_aligned(uint64_t alignment) {
    using scal::tlmalloc_aligned;
    void *mem = tlmalloc_aligned(sizeof(AtomicValue128<T>), alignment);
    AtomicValue128<T>* cp = new(mem) AtomicValue128<T>();
    return cp;
  }

  static const uint64_t kAbaMin = 0;
  static const uint64_t kAbaMax = UINT64_MAX;
  static const uint8_t  kAbaBits = 64;
  static const T        kValueMin;
  static const T        kValueMax;
  static const uint8_t  kValueBits = 64;

  inline AtomicValue128(void) {
    init(0, 0);
  }

  inline AtomicValue128(T value, uint64_t aba) {
    init(value, aba);
  }

  inline AtomicValue128(const AtomicValue128<T> &cpy) {
    memory_ = const_cast<AtomicValue128<T>&>(cpy).raw();
  }

  inline AtomicValue128(const volatile AtomicValue128<T> &cpy) {
    memory_ = const_cast<AtomicValue128<T>&>(cpy).raw();
  }

  inline AtomicValue128<T>& operator=(const AtomicValue128<T> &rhs) volatile {
    // Race in writing data.
    memory_ = const_cast<AtomicValue128<T>&>(rhs).raw();
    return const_cast<AtomicValue128<T>&>(*this);
  }

  inline AtomicValue128<T>& operator=(const volatile AtomicValue128<T> &rhs) volatile {
    // Race in writing data.
    memory_ = const_cast<AtomicValue128<T>&>(rhs).raw();
    return const_cast<AtomicValue128<T>&>(*this);
  }

  inline AtomicAba aba(void) const volatile {
    return raw() & UINT128_C(UINT64_MAX);
  }

  inline T value(void) const volatile {
    return (T)(raw() >> 64);
  }

  inline AtomicRaw raw(void) const volatile {
    uint128_t old;
    do {
      old = memory_;
    } while(memory_ != old);
    return old;
  }

  inline uint64_t weak_aba(void) const volatile {
    return memory_ & UINT128_C(UINT64_MAX);
  }

  inline T weak_value(void) const volatile {
    return (T)(memory_ >> 64);
  }

  inline AtomicRaw weak_raw(void) const volatile {
    return memory_;
  }

  inline void weak_set_value(T value) volatile {
    // Both, the read and the write operation, contain data races.
    uint128_t new_memory = memory_ & UINT128_C(UINT64_MAX);  // Delete old value, but keep ABA.
    memory_ = new_memory | (((uint128_t)value) << 64);
  }

  inline void weak_set_aba(uint64_t aba) volatile {
    // Both, the read and the write operation, contain data races.
    uint128_t new_memory = memory_ & (UINT128_C(UINT64_MAX) << 64);  // Delete ABA, but keep old value.
    memory_ = new_memory | aba;
  }

  inline void weak_set_raw(uint128_t raw) volatile {
    // Possible data race in here.
    memory_ = raw;
  }

  inline void set_value(T value) volatile {
    uint128_t old_memory;
    uint128_t new_memory;
    do {
      old_memory = raw();
      new_memory = old_memory & UINT128_C(UINT64_MAX);
      new_memory |= (((uint128_t)value) << 64);
    } while(!__sync_bool_compare_and_swap(&memory_, old_memory, new_memory));
  }

  inline void set_aba(uint64_t aba) volatile {
    uint128_t old_memory;
    uint128_t new_memory;
    do {
      old_memory = raw();
      new_memory = old_memory & (UINT128_C(UINT64_MAX) << 64);
      new_memory |= aba;
    } while(!__sync_bool_compare_and_swap(&memory_, old_memory, new_memory));
  }

  inline void set_raw(uint128_t new_raw) volatile {
    uint128_t old;
    do {
      old = raw();
    } while(!__sync_bool_compare_and_swap(&memory_, old, new_raw));
  }

  inline bool cas(AtomicValue128<T> &expected, AtomicValue128<T> &newcp) volatile {
    if (memory_ == expected.memory_ &&  // Filter out obvious fails.
        __sync_bool_compare_and_swap(&memory_, expected.memory_, newcp.memory_)) {
      return true;
    } 
    return false;
  }

 private:
  volatile uint128_t memory_;

  inline void init(T value, uint64_t aba) {
    if(sizeof(T) > 8) {
      fprintf(stderr , "%s: type T must be 8 bytes (64 bit) max!\n", __func__);
      abort();
    }
    memory_ = 0;
    weak_set_value(value);
    weak_set_aba(aba);
  }
};

template <typename T>
const T AtomicValue128<T>::kValueMin = (T)1;

template <typename T>
const T AtomicValue128<T>::kValueMax = (T)UINT64_MAX;

#endif  // SCAL_UTIL_ATOMIC_VALUE128_H_
