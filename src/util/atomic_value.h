#ifndef SCAL_UTIL_ATOMIC_VALUE_H_
#define SCAL_UTIL_ATOMIC_VALUE_H_

#define CAS_128 1

#ifdef CAS_128

#include "util/atomic_value128.h"
template<typename T>
using AtomicPointer = AtomicValue128<T>;
template<typename T>

using AtomicValue = AtomicValue128<T>;
#else  // no CAS_128

#include "util/atomic_value64_offset.h"
#include "util/atomic_value64_no_offset.h"
template<typename T>
using AtomicPointer = AtomicValue64Offset<T>;
template<typename T>
using AtomicValue = AtomicValue64NoOffset<T>;

#endif  // CAS_128

#endif  // SCAL_UTIL_ATOMIC_VALUE_H_
