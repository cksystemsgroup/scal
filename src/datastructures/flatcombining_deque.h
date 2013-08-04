// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the queue from:
//
// D. Hendler, I. Incze, N. Shavit, and M. Tzafrir. Flat combining and the
// synchronization-parallelism tradeoff. In Proceedings of the 22nd ACM
// symposium on Parallelism in algorithms and architectures, SPAA ’10, pages
// 355–364, New York, NY, USA, 2010. ACM.

#ifndef SCAL_DATASTRUCTURES_FLATCOMBINING_DEQUE_H_
#define SCAL_DATASTRUCTURES_FLATCOMBINING_DEQUE_H_

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructures/deque.h"
#include "datastructures/single_deque.h"
#include "util/malloc.h"
#include "util/threadlocals.h"
#include "util/operation_logger.h"

namespace fc_deque_details {

enum Opcode {
  Done = 0,
  Insert_Left = 1,
  Insert_Right = 2,
  Remove_Left = 4,
  Remove_Right = 8
};

template<typename T>
struct Operation {
  Opcode opcode;
  T data;
};
}  // fc_deque_details

template<typename T>
class FlatCombiningDeque : public Deque<T> {
 public:
  explicit FlatCombiningDeque(uint64_t num_threads);
  bool insert_left(T item);
  bool insert_right(T item);
  bool remove_left(T *item);
  bool remove_right(T *item);

 private:
  typedef fc_deque_details::Opcode Opcode;
  typedef fc_deque_details::Operation<T> Operation;

  uint64_t num_threads_;
  volatile fc_deque_details::Operation<T>* *operations_;
  SingleDeque<T> *deque_;
  bool *global_lock_;

  inline void set_op(uint64_t index, Opcode opcode, T data) {
    operations_[index]->data = data;
    operations_[index]->opcode = opcode;
  }

  inline bool is_op_done(uint64_t index) {
    return (operations_[index]->opcode == Opcode::Done);
  }

  void scan_combine_apply(void);
};

template<typename T>
FlatCombiningDeque<T>::FlatCombiningDeque(uint64_t num_threads) {
  num_threads_ = num_threads;
  operations_ = static_cast<volatile Operation**>(calloc(
      num_threads, sizeof(*operations_)));
  for (uint64_t i = 0; i < num_threads; i++) {
    operations_[i] = scal::get<Operation>(128);
  }
  deque_ = new SingleDeque<T>();
  global_lock_ = scal::get<bool>(128);
}

template<typename T>
void FlatCombiningDeque<T>::scan_combine_apply(void) {
  for (uint64_t i = 0; i < num_threads_; i++) {
    if (operations_[i]->opcode == Opcode::Insert_Left) {
      scal::StdOperationLogger::get_specific(i).linearization();
      deque_->insert_left(operations_[i]->data);
      set_op(i, Opcode::Done, (T)NULL);
    } else if (operations_[i]->opcode == Opcode::Insert_Right) {
      scal::StdOperationLogger::get_specific(i).linearization();
      deque_->insert_right(operations_[i]->data);
      set_op(i, Opcode::Done, (T)NULL);
    } else if (operations_[i]->opcode == Opcode::Remove_Left) {
      scal::StdOperationLogger::get_specific(i).linearization();
      T item;
      deque_->remove_left(&item);
      set_op(i, Opcode::Done, item);
    } else if (operations_[i]->opcode == Opcode::Remove_Right) {
      scal::StdOperationLogger::get_specific(i).linearization();
      T item;
      deque_->remove_right(&item);
      set_op(i, Opcode::Done, item);
    }

  }
}


template<typename T>
bool FlatCombiningDeque<T>::insert_left(T item) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  set_op(thread_id, Opcode::Insert_Left, item);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (is_op_done(thread_id)) {
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
bool FlatCombiningDeque<T>::insert_right(T item) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  set_op(thread_id, Opcode::Insert_Right, item);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (is_op_done(thread_id)) {
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
bool FlatCombiningDeque<T>::remove_left(T *item) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  set_op(thread_id, Opcode::Remove_Left, (T)NULL);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (is_op_done(thread_id)) {
        break;
      }
    } else {
      scan_combine_apply();
      *global_lock_ = false;
      break;
    }
  }
  *item = operations_[thread_id]->data;
  if (*item == (T)NULL) {
    return false;
  } else {
    return true;
  }
}

template<typename T>
bool FlatCombiningDeque<T>::remove_right(T *item) {
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  set_op(thread_id, Opcode::Remove_Right, (T)NULL);
  while (true) {
    if (!__sync_bool_compare_and_swap(global_lock_, false, true)) {
      if (is_op_done(thread_id)) {
        break;
      }
    } else {
      scan_combine_apply();
      *global_lock_ = false;
      break;
    }
  }
  *item = operations_[thread_id]->data;
  if (*item == (T)NULL) {
    return false;
  } else {
    return true;
  }
}

#endif  // SCAL_DATASTRUCTURES_FLATCOMBINING_DEQUE_H_
