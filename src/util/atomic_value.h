// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_ATOMIC_VALUE_H_
#define SCAL_UTIL_ATOMIC_VALUE_H_

// If we don't have a config.h file, just use the 128bit CAS (default).
#ifdef HAVE_CONFIG_H
#include "./config.h"
#else
#define USE_CAS128
#endif  // HAVE_CONFIG_H

#define SUPPORTS_TEMPLATE_ALIAS 1

#ifdef USE_CAS128

#include "util/atomic_value128.h"

#ifdef SUPPORTS_TEMPLATE_ALIAS
template<typename T> using AtomicPointer = AtomicValue128<T>;
template<typename T> using AtomicValue = AtomicValue128<T>;
#else
#define AtomicPointer AtomicValue128
#define AtomicValue AtomicValue128
#endif  // SUPPORTS_TEMPLATE_ALIAS

#else

#include "util/atomic_value64_offset.h"
#include "util/atomic_value64_no_offset.h"

#ifdef SUPPORTS_TEMPLATE_ALIAS
template<typename T> using AtomicPointer = AtomicValue64Offset<T>;
template<typename T> using AtomicValue = AtomicValue64NoOffset<T>;
#else
#define AtomicPointer AtomicValue64Offset
#define AtomicValue AtomicValue64NoOffset
#endif  // SUPPORTS_TEMPLATE_ALIAS

#endif  // USE_CAS128

#endif  // SCAL_UTIL_ATOMIC_VALUE_H_
