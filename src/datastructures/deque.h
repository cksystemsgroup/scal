// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_DEQUE_H_
#define SCAL_DATASTRUCTURES_DEQUE_H_

#include "datastructures/pool.h"
#include "datastructures/queue.h"

template<typename T>
class Deque : public Queue<T> {
 public:
  virtual bool insert_left(T item) = 0;
  virtual bool remove_left(T *item) = 0;

  virtual bool insert_right(T item) = 0;
  virtual bool remove_right(T *item) = 0;

  virtual inline bool enqueue(T item) {
    return insert_left(item);
  }

  virtual inline bool dequeue(T *item) {
    return remove_right(item);
  }
};

#endif  // SCAL_DATASTRUCTURES_QUEUE_H_
