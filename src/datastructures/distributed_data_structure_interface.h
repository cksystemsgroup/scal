// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_INTERFACE_H_
#define SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_INTERFACE_H_

#include "util/atomic_value_new.h"

template<typename T>
class DistributedDataStructureInterface {
 public:
  virtual bool put(T item) = 0;

  // Returns a state that is changed, as soon as an item is put into the data
  // structure. Thus, an observer recognizing empty at some point, can see if
  // the internal state (wrt. empty) changed.
  virtual AtomicRaw empty_state() = 0;

  // A get method, that additionally returns the state of the queue wrt to
  // getting an item. I.e. it returns that state where it observed empty, on a
  // failed dequeue.
  virtual bool get_return_empty_state(T *item, AtomicRaw *state) = 0;
};

#endif  // SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_INTERFACE_H_
