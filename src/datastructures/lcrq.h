// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// This is a wrapper around the upstream implementation of LCRQ. The files
// reside in
//   datastructures/upstream/lcrq
//
// The upstream files provide an implementation for:
//
// Fast Concurrent Queues for x86 Processors.  Adam Morrison and Yehuda Afek.
// PPoPP 2013.
//
// Website: http://mcg.cs.tau.ac.il/projects/lcrq/, accesed 01/28/2015

#ifndef SCAL_DATASTRUCTURES_LCRQ_H_
#define SCAL_DATASTRUCTURES_LCRQ_H_

#include "datastructures/queue.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
void lcrq_init();
bool lcrq_enqueue(uint64_t item);
bool lcrq_dequeue(uint64_t* item);
#ifdef __cplusplus
}
#endif  // __cplusplus

namespace scal {

template<typename T>
class LCRQ : public Queue<T> {
 public:
  inline LCRQ() {
    lcrq_init();
  }

  inline bool enqueue(T item) {
    return lcrq_enqueue(item);
  }

  inline bool dequeue(T* item) {
    return lcrq_dequeue(item);
  }
};

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_LCRQ_H_
