// Copyright (c) 2014, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_LOCK_H_
#define SCAL_UTIL_LOCK_H_

template<int PAD = 0>
class SpinLock {
 public:
  inline SpinLock() : lock_word_(0) {}

  inline void Lock() {
    while (!__sync_bool_compare_and_swap(&lock_word_, 0, 1)) {
      __asm__("PAUSE");
    }
  }

  inline bool TryLock() {
    return __sync_bool_compare_and_swap(&lock_word_, 0, 1);
  }

  inline void Unlock() {
    __sync_bool_compare_and_swap(&lock_word_, 1, 0);
  }

 private:
  intptr_t lock_word_;
  uint8_t _padding[ (PAD != 0) ?
      PAD - sizeof(lock_word_) : 0 ];
};


class LockHolder {
 public:
  explicit LockHolder(SpinLock<>* lock) : lock_(lock) {
    lock_->Lock();
  }

  ~LockHolder() {
    lock_->Unlock();
  }

 private:
  SpinLock<>* lock_;
};

#endif  // SCAL_UTIL_LOCK_H_
