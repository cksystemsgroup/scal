// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_PLATFORM_H_
#define SCAL_UTIL_PLATFORM_H_

namespace scal {

const uint64_t kPageSize = 4096;
const uint64_t kCachelineSize = 64;
const uint64_t kCachePrefetch = 128;

}  // namespace scal

#endif
