// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// This is just a wrapper to the original (upstream) implementation of the LCRQ,
// which can be found at http://mcg.cs.tau.ac.il/projects/lcrq
//
// The algorithm is described in:
//
// Fast Concurrent Queues for x86 Processors.  Adam Morrison and Yehuda Afek.
// PPoPP 2013.
//
// Note: Since this is just a wrapper, only one instance of this datastructure
// is supported!

#ifndef SCAL_DATASTRUCTURES_LCRQ_H_
#define SCAL_DATASTRUCTURES_LCRQ_H_

#include <new>

#include "datastructures/queue.h"
#include "datastructures/lcrq_upstream.h"

template<typename T>
class LCRQ : public Queue<T> {
 public:
  LCRQ();
  bool enqueue(T item);
  bool dequeue(T *item);
 private:
};

template<typename T>
LCRQ<T>::LCRQ() {
  lcrq_SHARED_OBJECT_INIT();
}

template<typename T>
bool LCRQ<T>::enqueue(T item) {
  lcrq_enqueue((long)item);
  return true;
}

template<typename T>
bool LCRQ<T>::dequeue(T* item) {
  *item = lcrq_dequeue();
  return true;
}

#endif  // SCAL_DATASTRUCTURES_LCRQ_H_
