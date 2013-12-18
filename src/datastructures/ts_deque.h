// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_DEQUE_H_
#define SCAL_DATASTRUCTURES_TS_DEQUE_H_

#include <stdint.h>
#include <atomic>

#include "datastructures/pool.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

template<typename T, typename TSBuffer, typename Timestamp>
class TSDeque : public Pool<T> {
  private:
 
    TSBuffer *buffer_;
    Timestamp *timestamping_;

  public:
    TSDeque (uint64_t num_threads, uint64_t delay) {

      timestamping_ = static_cast<Timestamp*>(
          scal::get<Timestamp>(scal::kCachePrefetch * 4));

      timestamping_->initialize(delay, num_threads);

      buffer_ = static_cast<TSBuffer*>(
          scal::get<TSBuffer>(scal:: kCachePrefetch * 4));
      buffer_->initialize(num_threads, timestamping_);
    }

    char* ds_get_stats(void) {
      return buffer_->ds_get_stats();
    }

    bool put(T element) {
      // Randomly insert an element either at the left or the right side
      // of the deque.
      if ((hwrand() % 2) == 0) {
        return insert_left(element);
      }
      return insert_right(element);
    }

    bool get(T *element) {
      // Randomly remove an element either at the left or the right side
      // of the deque.
      if ((hwrand() % 2) == 0) {
        return remove_left(element);
      }
      return remove_right(element);
    }

    bool insert_left(T element) {
      std::atomic<uint64_t> *item = buffer_->insert_left(element);
      // In the set_timestamp operation first a new timestamp is acquired
      // and then assigned to the item. The operation may not be executed
      // atomically.
      timestamping_->set_timestamp(item);
      return true;
    }

    bool insert_right(T element) {
      std::atomic<uint64_t> *item = buffer_->insert_right(element);
      // In the set_timestamp operation first a new timestamp is acquired
      // and then assigned to the item. The operation may not be executed
      // atomically.
      timestamping_->set_timestamp(item);
      return true;
    }

    bool remove_left(T *element) {
      // Read the invocation time of this operation, needed for the
      // elimination optimization.
      uint64_t invocation_time[2];
      timestamping_->read_time(invocation_time);
      while (buffer_->try_remove_left(element, invocation_time)) {

        if (*element != (T)NULL) {
          return true;
        }
      }
      // The deque was empty, return false.
      return false;
    }

    bool remove_right(T *element) {
      // Read the invocation time of this operation, needed for the
      // elimination optimization.
      uint64_t invocation_time[2];
      timestamping_->read_time(invocation_time);
      while (buffer_->try_remove_right(element, invocation_time)) {

        if (*element != (T)NULL) {
          return true;
        }
      }
      // The deque was empty, return false.
      return false;
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_DEQUE_H_

