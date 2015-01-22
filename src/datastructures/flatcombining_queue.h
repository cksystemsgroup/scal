// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the queue from:
//
// D. Hendler, I. Incze, N. Shavit, and M. Tzafrir. Flat combining and the
// synchronization-parallelism tradeoff. In Proceedings of the 22nd ACM
// symposium on Parallelism in algorithms and architectures, SPAA ’10, pages
// 355–364, New York, NY, USA, 2010. ACM.

#ifndef SCAL_DATASTRUCTURES_FLATCOMBINING_QUEUE_H_
#define SCAL_DATASTRUCTURES_FLATCOMBINING_QUEUE_H_

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/queue.h"
#include "datastructures/single_list.h"
#include "util/lock.h"
#include "util/malloc.h"
#include "util/threadlocals.h"

namespace scal {

namespace detail {

enum Opcode {
  Done = 0,
  Enqueue = 1,
  Dequeue = 2
};


template<typename T>
struct Operation {
  Operation() : opcode(Done) {}

  Opcode opcode;
  T data;

#define REST (128 - sizeof(T) - sizeof(Opcode))
  uint8_t pad1[REST];
#undef REST

  void* operator new(size_t size) {
    return MallocAligned(size, 128);
  }

  void* operator new[](size_t size) {
    return MallocAligned(size, 128);
  }
};

}  // namespace detail


template<typename T>
class FlatCombiningQueue : public Queue<T> {
 public:
  explicit FlatCombiningQueue(uint64_t num_ops);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef detail::Operation<T> Operation;
  typedef detail::Opcode Opcode;

  void ScanCombineApply();

  inline void SetOp(uint64_t index, Opcode opcode, T data) {
    operations_[index].data = data;
    operations_[index].opcode = opcode;
  }

  SpinLock<64> global_lock_;
  uint64_t num_ops_;
  SingleList<T> queue_;
  volatile Operation* operations_;
};


template<typename T>
FlatCombiningQueue<T>::FlatCombiningQueue(uint64_t num_ops)
    : num_ops_(num_ops),
      operations_(new Operation[num_ops]) {
}


template<typename T>
void FlatCombiningQueue<T>::ScanCombineApply() {
  for (uint64_t i = 0; i < num_ops_; i++) {
    if (operations_[i].opcode == Opcode::Enqueue) {
      queue_.enqueue(operations_[i].data);
      SetOp(i, Opcode::Done, (T)NULL);
    } else if (operations_[i].opcode == Opcode::Dequeue) {
      if (!queue_.is_empty()) {
        T item;
        queue_.dequeue(&item);
        SetOp(i, Opcode::Done, item);
      } else {
        SetOp(i, Opcode::Done, (T)NULL);
      }
    }
  }
}


template<typename T>
bool FlatCombiningQueue<T>::enqueue(T item) {
  const uint64_t thread_id = scal::ThreadContext::get().thread_id();
  SetOp(thread_id, Opcode::Enqueue, item);
  while (true) {
    if (!global_lock_.TryLock()) {
      if (operations_[thread_id].opcode == Opcode::Done) {
        return true;
      }
    } else {
      ScanCombineApply();
      global_lock_.Unlock();
      return true;
    }
  }
}


template<typename T>
bool FlatCombiningQueue<T>::dequeue(T *item) {
  const uint64_t thread_id = scal::ThreadContext::get().thread_id();
  SetOp(thread_id, Opcode::Dequeue, (T)NULL);
  while (true) {
    if (!global_lock_.TryLock()) {
      if (operations_[thread_id].opcode == Opcode::Done) {
        break;
      }
    } else {
      ScanCombineApply();
      global_lock_.Unlock();
      break;
    }
  }
  *item = operations_[thread_id].data;
  SetOp(thread_id, Opcode::Done, (T)NULL);
  if (*item == (T)NULL) {
    return false;
  } else {
    return true;
  }
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_FLATCOMBINING_QUEUE_H_
