// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_H_
#define SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_H_

#include <inttypes.h>

#include "datastructures/balancer.h"
#include "datastructures/distributed_data_structure_interface.h"
#include "datastructures/pool.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace scal {

template<typename T, class P, class B>
class DistributedDataStructure : public Pool<T> {
 public:
  DistributedDataStructure(
      size_t num_data_structures, uint64_t num_threads, B* balancer);
  bool put(T item);
  bool get(T *item);

 private:
  static const uint64_t kPtrAlignment = scal::kCachePrefetch;

  size_t num_data_structures_;
  B* balancer_;
  P **backend_;
};


template<typename T, class P, class B>
DistributedDataStructure<T, P, B>::DistributedDataStructure(
    size_t num_data_structures, uint64_t num_threads, B* balancer)
    : num_data_structures_(num_data_structures),
      balancer_(balancer) {
  backend_ = static_cast<P**>(calloc(num_data_structures_, sizeof(P*)));
  void* mem;
  for (uint64_t i = 0; i < num_data_structures_; i++) {
    mem = MallocAligned(sizeof(P), kPtrAlignment);
    backend_[i] = new (mem) P();
  }
}


template<typename T, class P, class B>
bool DistributedDataStructure<T, P, B>::put(T item) {
  const uint64_t index = balancer_->put_id();
  return backend_[index]->put(item);
}


template<typename T, class P, class B>
bool DistributedDataStructure<T, P, B>::get(T *item) {
  uint64_t start;

#ifdef GET_TRY_LOCAL_FIRST
  if (balancer_->local_get_id(&start) && backend_[start]->get(item)) {
    return true;
  }
#endif  // GET_TRY_LOCAL_FIRST

  size_t i;
  start = balancer_->get_id();
  size_t index;
  State tails[num_data_structures_];  // NOLINT
  while (true) {
    for (i = 0; i < num_data_structures_; i++) {
      index = (start + i) % num_data_structures_;
      if (backend_[index]->get_return_put_state(
              item, &(tails[index]))) {
        return true;
      }
    }
#ifdef NON_LINEARIZABLE_EMPTY
    return false;
#endif // NON_LINEARIZABLE_EMPTY
    for (i = 0; i < num_data_structures_; i++) {
      index = (start + i) % num_data_structures_;
      if (backend_[index]->put_state() != tails[index]) {
        start = index;
        break;
      }
      if (((index + 1) % num_data_structures_) == start) {
        return false;
      }
    }
  }
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_DISTRIBUTED_DATA_STRUCTURE_H_
