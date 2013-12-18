// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_QUEUE_H_
#define SCAL_DATASTRUCTURES_TS_QUEUE_H_

#include <stdint.h>
#include <atomic>

#include "datastructures/queue.h"
#include "util/malloc.h"
#include "util/platform.h"

template<typename T, typename TSBuffer, typename Timestamp>
class TSQueue : public Queue<T> {
  private:
 
    TSBuffer *buffer_;
    Timestamp *timestamping_;

  public:
    TSQueue (uint64_t num_threads, uint64_t delay) {

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

    bool enqueue(T element) {
      std::atomic<uint64_t> *item = buffer_->insert_left(element);
      // In the set_timestamp operation first a new timestamp is acquired
      // and then assigned to the item. The operation may not be executed
      // atomically.
      timestamping_->set_timestamp(item);
      return true;
    }

    bool dequeue(T *element) {
      uint64_t invocation_time[2];
      // The invocation time of this operation is only needed for the
      // elimination optimization, which is not possible for a queue.
      // Therefore we do not read the time to set the invocation time
      // but initialize it with TOP.
      timestamping_->init_top(invocation_time);
      while (buffer_->try_remove_right(element, invocation_time)) {

        if (*element != (T)NULL) {
          return true;
        }
      }
      // The queue was empty, return false.
      return false;
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_QUEUE_H_
