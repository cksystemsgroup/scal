// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ATOMIC_VALUE64_OFFSET_H_
#define SCAL_UTIL_ATOMIC_VALUE64_OFFSET_H_

#include <inttypes.h>

#include <limits>

#include "util/atomic_value64_base.h"

template<typename T>
class AtomicValue64Offset : public AtomicValue64Base<T> {
 public:
  static const AtomicAba  kAbaMin = 0;
  static const AtomicAba  kAbaMax = 0x7;
  static const uint8_t    kAbaBits = 3;
  static const T          kValueMin;
  static const T          kValueMax;
  static const uint8_t    kValueBits = 61;

  // deprecated
  static AtomicValue64Offset<T>* get_aligned(uint64_t alignment) {
    using scal::tlmalloc_aligned;
    void *mem = tlmalloc_aligned(sizeof(AtomicValue64Offset<T>), alignment);
    AtomicValue64Offset<T>* cp = new(mem) AtomicValue64Offset<T>();
    return cp;
  }

  inline AtomicValue64Offset() {
    init(0, 0);
  }

  inline AtomicValue64Offset(T value, AtomicAba aba) {
    init(value, aba);
  }

  inline AtomicValue64Offset(const AtomicValue64Offset<T> &cpy)
      : AtomicValue64Base<T>(cpy) {}

  inline AtomicValue64Offset(volatile const AtomicValue64Offset<T> &cpy)
      : AtomicValue64Base<T>(cpy) {}

  inline AtomicValue64Offset<T>& operator=(
      const AtomicValue64Offset<T> &rhs) volatile {
    this->memory_ = const_cast<AtomicValue64Offset<T>&>(rhs).raw();
    return const_cast<AtomicValue64Offset<T>&>(*this);
  }

  inline AtomicValue64Offset<T>& operator=(
      const volatile AtomicValue64Offset<T> &rhs) volatile {
    this->memory_ = const_cast<AtomicValue64Offset<T>&>(rhs).raw();
    return const_cast<AtomicValue64Offset<T>&>(*this);
  }

  inline T value(void) const volatile {
    return (T)(this->raw() &
        (std::numeric_limits<uint64_t>::max() - AtomicValue64Base<T>::kAbaMax));
  }

  inline void set_value(T value) volatile {
    uint64_t new_value = (uint64_t)value & 
        (std::numeric_limits<uint64_t>::max() - kAbaMax);
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
const T AtomicValue64Offset<T>::kValueMin =
    static_cast<T>(1 << AtomicValue64Offset<T>::kAbaBits);

template <typename T>
const T AtomicValue64Offset<T>::kValueMax = 
    static_cast<T>(std::numeric_limits<uint64_t>::max() - kAbaMax);

#endif  // SCAL_UTIL_ATOMIC_VALUE64_OFFSET_H_
