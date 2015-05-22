// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef DATASTRUCTURES_DYN_DISTRIBUTED_DATA_STRUCTURE_H_
#define DATASTRUCTURES_DYN_DISTRIBUTED_DATA_STRUCTURE_H_

#include "datastructures/pool.h"
#include "datastructures/distributed_data_structure_interface.h"
#include "util/allocation.h"
#include "util/lock.h"
#include "util/platform.h"
#include "util/random.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {

// A producer node (PNode) is always assigned to a producing thread, i.e., a
// thread that performed at least one put operation.
template<class P>
struct PNode : ThreadLocalMemory<64> {
  PNode() : alive(1), backend(NULL) {
    backend = new P();
  }

  int alive;
  P* backend;
};

}  // namespace detail


template<typename T, class P>
class DynamicDistributedDataStructure : public Pool<T> {
 public:
  DynamicDistributedDataStructure(uint64_t max_threads);
  bool put(T item);
  bool get(T* item);

  void Terminate();

 private:
  typedef detail::PNode<P> ProducerNode;

  ProducerNode* GetLocalNode(bool create_if_absent);
  void AnnounceThread(ProducerNode* node);
  void CleanupThread(uint64_t index);
  void CleanupThreadUnlocked(uint64_t index);

  uint64_t max_nodes_;
  ProducerNode** backends_;
  uint64_t p_;
  uint64_t ds_state_;
  SpinLock<> segment_lock_;
};


template<typename T, class P>
DynamicDistributedDataStructure<T, P>::DynamicDistributedDataStructure(uint64_t max_threads) 
    : max_nodes_(max_threads)
    , p_(0)
    , ds_state_(0) {
  const uint64_t backend_size = sizeof(ProducerNode*) * max_threads;
  backends_ = static_cast<ProducerNode**>(MallocAligned(backend_size, kPageSize));
  memset(backends_, 0, backend_size);
}


template<typename T, class P>
detail::PNode<P>* DynamicDistributedDataStructure<T, P>::GetLocalNode(bool create_if_absent) {
  scal::ThreadContext& ctx = scal::ThreadContext::get();
  void* data = ctx.get_data();
  if ((data == NULL) && create_if_absent) {
    ProducerNode* node = new ProducerNode();
    ctx.set_data(node);
    AnnounceThread(node);
    return node;
  }
  return reinterpret_cast<ProducerNode*>(data);
}


template<typename T, class P>
void DynamicDistributedDataStructure<T, P>::AnnounceThread(ProducerNode* node) {
  LockHolder l(&segment_lock_);

  if ((p_ + 1) == max_nodes_) {
    // Try to recover some nodes.
    for (uint64_t i = 0; i < p_; i++) {
      if ((backends_[i]->alive == 0) &&
          (backends_[i]->backend->empty())) {
        CleanupThreadUnlocked(i);
      }
    }
    if ((p_ + 1) == max_nodes_) {
      printf("cannot announce node as limit %lu is reached\n", max_nodes_);
      abort();
    }
  }

  backends_[p_] = node;
  p_++;
  ds_state_++;
}


template<typename T, class P>
void DynamicDistributedDataStructure<T, P>::CleanupThread(uint64_t index) {
  LockHolder l(&segment_lock_);
  CleanupThreadUnlocked(index);
}


template<typename T, class P>
void DynamicDistributedDataStructure<T, P>::CleanupThreadUnlocked(uint64_t index) {
  ProducerNode* n = backends_[index];
  if ((n == NULL) || ((n->alive == 1) || !n->backend->empty())) {
    return;
  }

  backends_[index] = backends_[p_-1];
  backends_[p_-1] = NULL;
  p_--;
  ds_state_++;
}


template<typename T, class P>
bool DynamicDistributedDataStructure<T, P>::put(T item) {
  ProducerNode* node = GetLocalNode(true);
  return node->backend->put(item);
}


template<typename T, class P>
bool DynamicDistributedDataStructure<T, P>::get(T* item) {
  ProducerNode* node = GetLocalNode(false);
  if ((node != NULL) && node->backend->get(item)) {
    // Fast path: We just get an item from our local backend.
    return true;
  }

  // nothing in local backend, try random.
  uint64_t start;
  size_t index;
  size_t i;
  uint64_t old_ds_state;
  uint64_t len;
  bool retry;

  while (true) {
    len = p_;
    if (len == 0) { return false; }
    start = pseudorand() % len;
    old_ds_state = ds_state_; 

    State tails[len];  // NOLINT
    retry = false;

    for (i = 0; i < len; i++) {
      index = (start + i) % len;
      ProducerNode* node = backends_[index];
      if ((node != NULL) &&
          node->backend->get_return_put_state(item, &tails[i])) {
        return true;
      } else {
        if ((node != NULL) && (node->alive == 0)) {
          //CleanupThread(node);
          CleanupThread(index);
          retry = true;
          break;
        }
      }
    }

#ifdef NON_LINEARIZABLE_EMPTY
    return false;
#endif  // NON_LINEARIZABLE_EMPTY

    if (retry) { continue; }
    if (old_ds_state != ds_state_) { continue; }

    for (i = 0; i < len; i++) {
      index = (start + i) % len;
      ProducerNode* node = backends_[index];
      if ((node != NULL) &&
          (node->backend->put_state() != tails[i])) {
        //start = index;
        retry = true;
        break;
      }
    }

    if (retry) { continue; }
    return false;
  }
}


template<typename T, class P>
void DynamicDistributedDataStructure<T, P>::Terminate() {
  ProducerNode* const node = GetLocalNode(false);
  if (node == NULL) {
    return;
  }

  // For scal:: we need to get rid of our threadlocal state.
  scal::ThreadContext& ctx = scal::ThreadContext::get();
  ctx.set_data(NULL);

  node->alive = 0;
  if (node->backend->empty()) {
    for (uint64_t i = 0; i < p_; i++) {
      if (backends_[i] == node) {
        CleanupThread(i);
        break;
      } 
    }
  }
}

}  // namespace scal

#endif  // DATASTRUCTURES_DYN_DISTRIBUTED_DATA_STRUCTURE_H_
