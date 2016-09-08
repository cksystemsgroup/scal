// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_STACK_H_
#define SCAL_DATASTRUCTURES_TS_STACK_H_

#include <inttypes.h>
#include <atomic>

#include "datastructures/stack.h"
#include "util/malloc.h"
#include "util/platform.h"

template<typename T, typename TSBuffer, typename Timestamp>
class TSStack : public Stack<T> {
  private:
 
    TSBuffer *buffer_;
    Timestamp *timestamping_;

  public:
    TSStack (uint64_t num_threads, uint64_t delay) {

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

    inline bool push(T element) {
      std::atomic<uint64_t> *item = buffer_->insert_right(element);
      timestamping_->set_timestamp(item);
      return true;
    }

    inline bool pop(T *element) {
      // Read the invocation time of this operation, needed for the
      // elimination optimization.
      uint64_t invocation_time[2];
      timestamping_->read_time(invocation_time);
      while (buffer_->try_remove_right(element, invocation_time)) {

        if (*element != (T)NULL) {
          return true;
        }
      }
      // The stack was empty, return false.
      return false;
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_DATASTRUCTURE_H_

