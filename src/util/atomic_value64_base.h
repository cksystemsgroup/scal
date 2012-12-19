// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ATOMIC_VALUE64_BASE_H_
#define SCAL_UTIL_ATOMIC_VALUE64_BASE_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/malloc.h"

typedef uint8_t AtomicAba;
typedef uint64_t AtomicRaw;

template<typename T>
class AtomicValue64Base {
 public:
  static const uint8_t kAbaMin = 0;
  static const uint8_t kAbaMax = 0xF;
  static const uint8_t kAbaBits = 4;

  inline AtomicValue64Base(void) {
    memory_ = 0;
  }

  inline AtomicValue64Base(const AtomicValue64Base<T> &cpy) {
    memory_ = const_cast<AtomicValue64Base<T>&>(cpy).raw();
  }

  inline AtomicValue64Base(const volatile AtomicValue64Base<T> &cpy) {
    memory_ = const_cast<AtomicValue64Base<T>&>(cpy).raw();
  }

  inline AtomicValue64Base<T>& operator=(
      const AtomicValue64Base<T> &rhs) volatile {
    memory_ = const_cast<AtomicValue64Base<T>&>(rhs).raw();
    return const_cast<AtomicValue64Base<T>&>(*this);
  }

  inline AtomicValue64Base<T>& operator=(
      const volatile AtomicValue64Base<T> &rhs) volatile {
    memory_ = const_cast<AtomicValue64Base<T>&>(rhs).raw();
    return const_cast<AtomicValue64Base<T>&>(*this);
  }

  inline AtomicAba aba(void) const volatile {
    return raw() & kAbaMax;
  }

  inline AtomicRaw raw(void) const volatile {
    return memory_;
  }

  inline void set_aba(AtomicAba aba) volatile {
    uint64_t new_memory = memory_;
    new_memory &= (UINT64_MAX - kAbaMax);  // Clear out ABA
    new_memory |= (aba & kAbaMax);  // Add the new (masked) ABA
    memory_ = new_memory;
  }

  inline void set_raw(AtomicRaw new_raw) volatile {
    memory_ = new_raw;
  }

  inline void weak_set_value(T value) volatile {
    set_value(value);
  }

  inline void weak_set_aba(AtomicAba aba) volatile {
    set_aba(aba);
  }

  inline bool cas(AtomicValue64Base<T> &expected,
                  AtomicValue64Base<T> &newcp) volatile {
    if (memory_ == expected.memory_ &&  // Filter out obvious fails.
        __sync_bool_compare_and_swap(&memory_,
                                     expected.memory_,
                                     newcp.memory_)) {
      return true;
    }
    return false;
  }

  virtual T value(void) const volatile = 0;
  virtual void set_value(T value) volatile = 0;
  virtual ~AtomicValue64Base() {}

 protected:
  volatile uint64_t memory_;
};

#endif  // SCAL_UTIL_ATOMIC_VALUE64_BASE_H_
