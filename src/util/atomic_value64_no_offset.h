// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ATOMIC_VALUE64_NO_OFFSET_H_
#define SCAL_UTIL_ATOMIC_VALUE64_NO_OFFSET_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>

#include "util/atomic_value64_base.h"

template<typename T>
class AtomicValue64NoOffset : public AtomicValue64Base<T> {
 public:
  static const uint64_t kAbaMin = 0;
  static const uint64_t kAbaMax = 0xF;
  static const uint8_t  kAbaBits = 4;
  static const T        kValueMin;
  static const T        kValueMax;
  static const uint8_t  kValueBits = 60;

  // deprecated
  static AtomicValue64NoOffset<T>* get_aligned(uint64_t alignment) {
    using scal::tlmalloc_aligned;
    void *mem = tlmalloc_aligned(sizeof(AtomicValue64NoOffset<T>), alignment);
    AtomicValue64NoOffset<T>* cp = new(mem) AtomicValue64NoOffset<T>();
    return cp;
  }

  inline AtomicValue64NoOffset() {
    init(0, 0);
  }

  inline AtomicValue64NoOffset(T value, AtomicAba aba) {
    init(value, aba);
  }

  inline AtomicValue64NoOffset(const AtomicValue64NoOffset<T> &cpy)
      : AtomicValue64Base<T>(cpy) {}

  inline AtomicValue64NoOffset(const volatile AtomicValue64NoOffset<T> &cpy)
      : AtomicValue64Base<T>(cpy) {}

  inline AtomicValue64NoOffset<T>& operator=(
      const AtomicValue64NoOffset<T> &rhs) volatile {
    this->memory_ = const_cast<AtomicValue64NoOffset<T>&>(rhs).raw();
    return const_cast<AtomicValue64NoOffset<T>&>(*this);
  }

  inline AtomicValue64NoOffset<T>& operator=(
      const volatile AtomicValue64NoOffset<T> &rhs) volatile {
    this->memory_ = const_cast<AtomicValue64NoOffset<T>&>(rhs).raw();
    return const_cast<AtomicValue64NoOffset<T>&>(*this);
  }

  inline T value(void) const volatile {
    return (T)(this->raw() >> AtomicValue64Base<T>::kAbaBits);
  }

  inline void set_value(T value) volatile {
    uint64_t new_value = (uint64_t)value << kAbaBits;
    uint64_t new_memory = this->memory_;
    new_memory &= kAbaMax;
    new_memory |= new_value;
    this->memory_ = new_memory;
  }

 private:
  inline void init(T value, AtomicAba aba) {
    this->memory_ = 0;
    set_value(value);
    this->set_aba(aba);
  }
};

template <typename T>
const T AtomicValue64NoOffset<T>::kValueMin = (T)1;

template <typename T>
const T AtomicValue64NoOffset<T>::kValueMax = (T)(UINT64_MAX >> 4);

#endif  // SCAL_UTIL_ATOMIC_VALUE64_NO_OFFSET_H_
