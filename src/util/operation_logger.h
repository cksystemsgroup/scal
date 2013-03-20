// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_OPERATION_LOGGER_H_
#define SCAL_UTIL_OPERATION_LOGGER_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/malloc.h"
#include "util/platform.h"
#include "util/time.h"
#include "util/threadlocals.h"

namespace scal {

enum LogType {
  kDequeue = 0,
  kEnqueue
};

char const kLogTypeSymbols[] = { '+', '-' };

template<typename T>
struct Operation {
  uint64_t op_type;
  uint64_t invocation;
  uint64_t response;
  uint64_t linearization;
  bool success;
  T item;
};

template<typename T>
class TLOperationLoggerInterface {
 public:
  virtual void invoke(uint64_t type) = 0;
  virtual void response(bool success, T item) = 0;
  virtual void linearization() = 0;
  virtual ~TLOperationLoggerInterface() {}
};

template<typename T>
class TLOperationLoggerNoop : public TLOperationLoggerInterface<T> {
 public:
  virtual inline void invoke(uint64_t type) {}
  virtual inline void response(bool success, T item) {}
  virtual inline void linearization() {}
};

template<typename T>
class TLOperationLogger : public TLOperationLoggerInterface<T> {
 public:
  TLOperationLogger() {} 

  void init(uint64_t num_ops) {
    count_ = 0;
    operations_ = static_cast<Operation<T>*>(scal::malloc_aligned(
        num_ops * sizeof(Operation<T>), kPageSize));
    memset(operations_, 0, num_ops * sizeof(Operation<T>));
  }

  inline void invoke(uint64_t type) {
    Operation<T> *op = &operations_[count_];
    op->op_type = type;
    op->invocation = get_hwtime();
  }

  inline void response(bool success, T item) {
    Operation<T> *op = &operations_[count_];
    op->success = success;
    op->response = get_hwtime();
    op->item = item;
    count_++;
  }

  inline void linearization() {
    Operation<T> *op = &operations_[count_];
    op->linearization = get_hwtime();
  }

  void print_summary() {
    for (uint64_t i = 0; i < count_; i++) {
      Operation<T> *op = &operations_[i];
      if (!op->success) {
        op->item = 0;
      }
      printf("%c %lu %lu %lu %lu\n",
          kLogTypeSymbols[op->op_type],
          op->item,
          op->invocation,
          op->linearization,
          op->response);
    }
  }

 private:
  uint64_t count_;
  Operation<T> *operations_;
};

template<typename T>
class OperationLogger {
 public:
  static void prepare(uint64_t num_threads, uint64_t num_ops) {
    num_loggers_ = num_threads;
    tl_loggers_ = static_cast<TLOperationLogger<T>**>(calloc(
        num_threads, sizeof(TLOperationLogger<T>*))); 
    for (uint64_t i = 0; i < num_threads; i++) {
      tl_loggers_[i] = scal::get<TLOperationLogger<T>>(kPageSize);
      tl_loggers_[i]->init(num_ops);
    }
    active_ = true;
  }

  static inline TLOperationLoggerInterface<T>& get(void) {
    if (!active_) {
      return noop_logger_;
    }
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    return *(tl_loggers_[thread_id]);
  }

  static inline TLOperationLoggerInterface<T>& get_specific(
      uint64_t thread_id) {
    if (!active_) {
      return noop_logger_;
    }
    return *(tl_loggers_[thread_id]);
  }

  static void print_summary(void) {
    if (!active_) {
      return;
    }
    for (uint64_t i = 0; i < num_loggers_; i++) {
      tl_loggers_[i]->print_summary();
    }
  }

 private:
  static TLOperationLogger<T> **tl_loggers_;
  static uint64_t num_loggers_;
  static bool active_;
  static TLOperationLoggerNoop<T> noop_logger_;
};

template<typename T>
TLOperationLogger<T>** OperationLogger<T>::tl_loggers_ = NULL;

template<typename T>
uint64_t OperationLogger<T>::num_loggers_ = 0;

template<typename T>
bool OperationLogger<T>::active_ = false;

template<typename T>
TLOperationLoggerNoop<T> OperationLogger<T>::noop_logger_;

class StdOperationLogger : public OperationLogger<uint64_t> {};

}  // namespace scal

#endif  // SCAL_UTIL_OPERATION_LOGGER_H_
