// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "util/workloads.h"

#include <inttypes.h>

// iteratively compute pi
// the greater n, the better the approximation
double calculate_pi(int n) {
  int f = 1 - n;
  int ddF_x = 1;
  int ddF_y = -2 * n;
  int x = 0;
  int y = n;
  int64_t in = 0;
  while (x < y) {
    if (f >= 0) {
      --y;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    in += y - x;
  }
  return 8.0 * in / (static_cast<double>(n) * n);
}
