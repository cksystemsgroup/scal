// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef DATASTRUCTURES_DYN_DISTRIBUTED_QUEUE_H_
#define DATASTRUCTURES_DYN_DISTRIBUTED_QUEUE_H_

#include "datastructures/pool.h"
#include "util/allocation.h"
#include "util/lock.h"
#include "util/platform.h"
#include "util/random.h"
#include "datastructures/single_list.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {


enum class Operation {
  kDone,
  kDequeue
};


// Each thread is assigneda single backend node that holds a backend and the
// flat-combining flag, results.
template<class B, typename T>
struct BackendNode {
  BackendNode() : alive(1) {
    backend = new B();
    operation_status = Operation::kDone;
  }

  // Backend.
  B* backend;

  // Alive or dead.
  int alive;

  // The flat-combining operation.
  Operation operation_status;

  // The resulting item for a flat-combining dequeue.
  T item;
};

}  // namespace detail


template<typename T, class P>
class LocLinFlatcombiningQueue : public Pool<T> {
 public:
  LocLinFlatcombiningQueue(uint64_t max_threads);
  bool put(T item);
  bool get(T* item);

  void Terminate();

 private:
  typedef detail::BackendNode<P, T> BackendNode;

  BackendNode* GetLocalNode();
  void AnnounceThread(BackendNode* node);
  void CleanupThread(uint64_t index);
  void CleanupThreadUnlocked(uint64_t index);

  bool DistributedQueueDequeue(T* item);

  uint64_t max_nodes_;
  BackendNode** backends_;
  uint64_t p_;
  SpinLock<> segment_lock_;
};


template<typename T, class P>
LocLinFlatcombiningQueue<T, P>::LocLinFlatcombiningQueue(uint64_t max_threads) 
    : max_nodes_(max_threads)
    , p_(0) {
  const uint64_t backend_size = sizeof(BackendNode*) * max_threads;
  backends_ = static_cast<BackendNode**>(MallocAligned(backend_size, kPageSize));
  memset(backends_, 0, backend_size);
}


template<typename T, class P>
detail::BackendNode<P, T>* LocLinFlatcombiningQueue<T, P>::GetLocalNode() {
  scal::ThreadContext& ctx = scal::ThreadContext::get();
  void* data = ctx.get_data();
  if (data == NULL) {
    BackendNode* node = new BackendNode();
    ctx.set_data(node);
    AnnounceThread(node);
    return node;
  }
  return static_cast<BackendNode*>(data);
}


template<typename T, class P>
void LocLinFlatcombiningQueue<T, P>::AnnounceThread(BackendNode* node) {
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
}


template<typename T, class P>
void LocLinFlatcombiningQueue<T, P>::CleanupThread(uint64_t index) {
  LockHolder l(&segment_lock_);
  CleanupThreadUnlocked(index);
}


template<typename T, class P>
void LocLinFlatcombiningQueue<T, P>::CleanupThreadUnlocked(uint64_t index) {
  BackendNode* n = backends_[index];
  if ((n == NULL) || ((n->alive == 1) || !n->backend->empty())) {
    return;
  }

  backends_[index] = backends_[p_-1];
  backends_[p_-1] = NULL;
  p_--;
}


template<typename T, class P>
bool LocLinFlatcombiningQueue<T, P>::put(T item) {
  BackendNode* node = GetLocalNode();
  return node->backend->enqueue(item);
}


template<typename T, class P>
bool LocLinFlatcombiningQueue<T, P>::get(T* item) {
  BackendNode* node = GetLocalNode();
  node->operation_status = detail::Operation::kDequeue;
  while (true) {
    if (segment_lock_.TryLock()) {
      for (uint64_t i = 0; i < p_; i++) {
        if (backends_[i]->operation_status == detail::Operation::kDequeue) {
          T item;
          if (DistributedQueueDequeue(&item)) {
            backends_[i]->item = item;
          } else {
            backends_[i]->item = (T)NULL;
          }
          backends_[i]->operation_status = detail::Operation::kDone;
        }
      }
      segment_lock_.Unlock();
      break;
    } else {
      // maybe put in a backoff
      __asm__("PAUSE");
      if (node->operation_status == detail::Operation::kDone) {
        break;
      }
    }
  }
  *item = node->item;
  if (*item == (T)NULL) {
    return false;
  }

  return true;
}

template<typename T, class P>
bool LocLinFlatcombiningQueue<T, P>::DistributedQueueDequeue(T* item) {
  BackendNode* node = GetLocalNode();
  if ((node != NULL) && node->backend->get(item)) {
    // Fast path: We just get an item from our local backend.
    return true;
  }

  // nothing in local backend, try random.
  uint64_t start;
  size_t index;
  size_t i;
  uint64_t len;

  while (true) {
    len = p_;
    if (len == 0) { return false; }
    start = pseudorand() % len;
    for (i = 0; i < len; i++) {
      index = (start + i) % len;
      BackendNode* node = backends_[index];
      if ((node != NULL) && node->backend->get(item)) {
        return true;
      }
      // TODO: cleanup unused (dead) nodes.
    }
    return false;
  }
}


template<typename T, class P>
void LocLinFlatcombiningQueue<T, P>::Terminate() {
  // TODO
}


}  // namespace scal

#endif  // DATASTRUCTURES_DYN_DISTRIBUTED_QUEUE_H_
