// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_QUEUE_H_
#define SCAL_DATASTRUCTURES_QUEUE_H_

#include "datastructures/pool.h"

template<typename T>
class Queue : public Pool<T> {
 public:
  virtual bool enqueue(T item) = 0;
  virtual bool dequeue(T *item) = 0;

  inline bool put(T item) {
    return enqueue(item);
  }

  inline bool get(T *item) {
    return dequeue(item);
  }
};

#endif  // SCAL_DATASTRUCTURES_QUEUE_H_

