// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_LOCKBASED_QUEUE_
#define SCAL_DATASTRUCTURES_LOCKBASED_QUEUE_

#include <assert.h>
#include <errno.h>      // ETIMEDOUT
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // strerror_r
#include <sys/time.h>   // gettimeofday

#include "datastructures/queue.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/threadlocals.h"
#include "util/operation_logger.h"

namespace lb_details {

template<typename T>
struct Node {
  T value;
  Node<T> *next;
};

}  // namespace lb_details

template<typename T>
class LockBasedQueue : Queue<T> {
 public:
  LockBasedQueue(uint64_t dequeue_mode, uint64_t dequeue_timeout);
  bool enqueue(T item);

  inline bool dequeue(T *item) {
    switch (dequeue_mode_) {
    case 0:
      return dequeue_default(item);
    case 1:
      return dequeue_blocking(item);
    case 2:
      return dequeue_timeout(item, dequeue_timeout_);
    default:
      return dequeue_default(item);
    }
  }

 private:
  typedef lb_details::Node<T> Node;

  static const uint8_t kPtrAlignment = scal::kCachePrefetch;

  Node *head_;
  Node *tail_;
  pthread_mutex_t *global_lock_;
  pthread_cond_t *enqueue_cond_;
  uint64_t dequeue_mode_;
  uint64_t dequeue_timeout_;

  inline void check_error(const char *std, int rc) {
    if (rc != 0) {
      char err[256];
      char* result = strerror_r(rc, err, 256);
      fprintf(stderr, "error: %s: %s\n", std, err);
      abort();
    }
  }

  bool dequeue_default(T *item);
  bool dequeue_blocking(T *item);
  bool dequeue_timeout(T *item, uint64_t timeout_ms);
};

template<typename T>
LockBasedQueue<T>::LockBasedQueue(uint64_t dequeue_mode,
                                  uint64_t dequeue_timeout) {
  global_lock_ = scal::get<pthread_mutex_t>(kPtrAlignment);
  int rc = pthread_mutex_init(global_lock_, NULL);
  check_error("pthread_mutex_init", rc);
  enqueue_cond_ = scal::get<pthread_cond_t>(kPtrAlignment);
  rc = pthread_cond_init(enqueue_cond_, NULL);
  check_error("pthread_cond_init", rc);
  Node *node = scal::get<Node>(kPtrAlignment);
  head_ = node;
  tail_ = node;
  dequeue_mode_ = dequeue_mode;
  dequeue_timeout_ = dequeue_timeout;
}

template<typename T>
bool LockBasedQueue<T>::enqueue(T item) {
  Node *node = scal::tlget<Node>(kPtrAlignment);
  node->value = item;
  int rc = pthread_mutex_lock(global_lock_);
  scal::StdOperationLogger::get().linearization();
  check_error("pthread_mutex_lock", rc);
  Node *tail_old = tail_;
  tail_old->next = node;
  tail_ = node;
  rc = pthread_cond_broadcast(enqueue_cond_);
  check_error("pthread_cond_broadcast", rc);
  rc = pthread_mutex_unlock(global_lock_);
  check_error("pthread_mutex_unlock", rc);
  return true;
}

template<typename T>
bool LockBasedQueue<T>::dequeue_default(T *item) {
  int rc = pthread_mutex_lock(global_lock_);
  scal::StdOperationLogger::get().linearization();
  check_error("pthread_mutex_lock", rc);
  if (head_ == tail_) {
    pthread_mutex_unlock(global_lock_);
    check_error("pthread_mutex_unlock", rc);
    return false;
  }
  *item = head_->next->value;
  head_ = head_->next;
  rc = pthread_mutex_unlock(global_lock_);
  check_error("pthread_mutex_unlock", rc);
  return true;
}

template<typename T>
bool LockBasedQueue<T>::dequeue_blocking(T *item) {
  int rc;
  while (true) {
    rc = pthread_mutex_lock(global_lock_);
    scal::StdOperationLogger::get().linearization();
    check_error("pthread_mutex_lock", rc);
    while (head_ == tail_) {
      pthread_cond_wait(enqueue_cond_, global_lock_);
    }
    assert(head_ != tail_);
    *item = head_->next->value;
    head_ = head_->next;
    rc = pthread_mutex_unlock(global_lock_);
    check_error("pthread_mutex_unlock", rc);
    return true;
  }
}

template<typename T>
bool LockBasedQueue<T>::dequeue_timeout(T *item, uint64_t timeout_ms) {
  int rc;
  struct timeval tp;
  struct timespec ts;
  uint64_t tmp_nsec;

  rc = gettimeofday(&tp, NULL);
  check_error("gettimeofday", rc);
  ts.tv_sec = tp.tv_sec;
  ts.tv_nsec = tp.tv_usec * 1000;
  tmp_nsec = ts.tv_nsec;
  tmp_nsec += timeout_ms * 1000000;
  if (tmp_nsec >= 1000000000) {
    ts.tv_sec += tmp_nsec / 1000000000;
    ts.tv_nsec = tmp_nsec % 1000000000;
  } else {
     ts.tv_nsec = tmp_nsec;
  }

  while (true) {
    rc =pthread_mutex_lock(global_lock_);
    scal::StdOperationLogger::get().linearization();
    check_error("pthread_mutex_lock", rc);
    while (head_ == tail_) {
      rc = pthread_cond_timedwait(enqueue_cond_, global_lock_, &ts);
      if (rc == ETIMEDOUT) {
        rc = pthread_mutex_unlock(global_lock_);
        check_error("pthread_mutex_unlock", rc);
        return false;
      }
      check_error("pthread_cond_timedwait", rc);
    }
    assert(head_ != tail_);
    *item = head_->next->value;
    head_ = head_->next;
    rc = pthread_mutex_unlock(global_lock_);
    check_error("pthread_mutex_unlock", rc);
    return true;
  }
}

#endif  // SCAL_DATASTRUCTURES_LOCKBASED_QUEUE_
