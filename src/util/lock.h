// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_LOCK_H_
#define SCAL_UTIL_LOCK_H_

class SpinLock {
 public:
  SpinLock() : lock_word_(0) {}

  void Lock() {
    while (!__sync_bool_compare_and_swap(&lock_word_, 0, 1)) {
      __asm__("PAUSE");
    }
  }

  void Unlock() {
    __sync_bool_compare_and_swap(&lock_word_, 1, 0);
  }

 private:
  intptr_t lock_word_ ;
};


class LockHolder {
 public:
  LockHolder(SpinLock* lock) : lock_(lock) {
    lock_->Lock();
  }

  ~LockHolder() {
    lock_->Unlock();
  }

 private:
  SpinLock* lock_;
};

#endif  // SCAL_UTIL_LOCK_H_
