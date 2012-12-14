// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_SCHED_H_
#define SCAL_UTIL_SCHED_H_

#include <gflags/gflags.h>
#include <sched.h>

DEFINE_bool(set_rt_priority, true,
            "try to set the program to RT priority (needs root)");

inline void try_set_program_prio(uint32_t prio) {
  if (FLAGS_set_rt_priority) {
    struct sched_param param;
    param.sched_priority = 40;
    if (sched_setscheduler(0, SCHED_RR, &param)) {
      fprintf(stderr,
              "WARNING: could not set RT priority, "
              "continue with normal priority\n");
    }
  }
}

#endif  // SCAL_UTIL_SCHED_H_
