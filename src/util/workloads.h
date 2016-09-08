// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_WORKLOADS_H_
#define SCAL_UTIL_WORKLOADS_H_

#include <inttypes.h>

#include "util/platform.h"

namespace scal {

double ComputePi(uint64_t n);

void RdtscWait(uint64_t n);

}  // namespace scal

#endif  // SCAL_UTIL_WORKLOADS_H_
