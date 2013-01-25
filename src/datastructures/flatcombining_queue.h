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
#include "util/malloc.h"
#include "util/threadlocals.h"

namespace fc_details {

enum Opcode {
  Done = 0,
  Enqueue = 1,
  Dequeue = 2
};

template<typename T>
struct Operation {
  Opcode opcode;
  T data;
};
}  // fc_details

template<typename T>
class FlatCombiningQueue : public Queue<T> {
 public:
  explicit FlatCombiningQueue(uint64_t num_ops);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef fc_details::Opcode Opcode;
  typedef fc_details::Operation<T> Operation;

  uint64_t num_ops_;
  volatile fc_details::Operation<T>* *operations_;
  SingleList<T> *queue_;
  bool *global_lock_;

  inline void set_op(uint64_t index, Opcode opcode, T data) {
    operations_[index]->data = data;
    operations_[index]->opcode = opcode;
  }

  void scan_combine_apply(void);
};

template<typename T>
FlatCombiningQueue<T>::FlatCombiningQueue(uint64_t num_ops) {
  num_ops_ = num_ops;
  operations_ = static_cast<volatile Operation**>(calloc(
      num_ops, sizeof(*operations_)));
  for (uint64_t i = 0; i < num_ops; i++) {
    operations_[i] = scal::get<Operation>(128);
  }
  queue_ = new SingleList<T>();
  global_lock_ = scal::get<bool>(128);
}

template<typename T>
void FlatCombiningQueue<T>::scan_combine_apply(void) {
  for (uint64_t i = 0; i < num_ops_; i++) {
    if (operations_[i]->opcode == Opcode::Enqueue) {
      queue_->enqueue(operations_[i]->data);
      set_op(i, Opcode::Done, (T)NULL);
    } else if (operations_[i]->opcode == Opcode::Dequeue) {
      if (!queue_->is_empty()) {
        T item;
        queue_->dequeue(&item);
        set_op(i, Opcode::Done, item);
      } else {
        set_op(i, Opcode::Done, (T)NULL);
      }
    }
  }
}


template<typename T>
bool FlatCombiningQueue<T>::enqueue(T item) {
  uint64_t thread_id = threadlocals_get()->thread_id;
  set_op(thread_id, Opcode::Enqueue, item);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (operations_[thread_id]->opcode == Opcode::Done) {
        return true;
      }
    } else {
      scan_combine_apply();
      *global_lock_ = false;
      return true;
    }
  }
}

template<typename T>
bool FlatCombiningQueue<T>::dequeue(T *item) {
  uint64_t thread_id = threadlocals_get()->thread_id;
  set_op(thread_id, Opcode::Dequeue, (T)NULL);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (operations_[thread_id]->opcode == Opcode::Done) {
        break;
      }
    } else {
      scan_combine_apply();
      *global_lock_ = false;
      break;
    }
  }
  *item = operations_[thread_id]->data;
  set_op(thread_id, Opcode::Done, (T)NULL);
  if (*item == (T)NULL) {
    return false;
  } else {
    return true;
  }
}

#endif  // SCAL_DATASTRUCTURES_FLATCOMBINING_QUEUE_H_
