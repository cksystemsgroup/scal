// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_UTIL_BITMAP_H_
#define SCAL_UTIL_BITMAP_H_

#include <inttypes.h>

class Bitmap256 {
 public:
  Bitmap256() {
    reset();
  }

  inline void reset() {
    map1_ = 0;
    map2_ = 0;
    map3_ = 0;
    map4_ = 0;
  }

  inline void set(uint8_t index) {
    if (index < 64) {
      map1_ |= (1u << index);
    } else if (index < 128) {
      map2_ |= (1u << (index - 64));
    } else if (index < 192) {
      map3_ |= (1u << (index - 128));
    } else {
      map4_ |= (1u << (index - 192));
    }
  }

  inline bool someset() {
    if (map1_ > 0 || map2_ > 0 || map3_ > 0 || map4_ > 0) {
      return true;
    }
    return false;
  }

  inline bool isset(uint8_t index) {
    if (index < 64 && (map1_ & (1ul << index))) {
      return true; 
    } else if (index >= 64 && index < 128 && (map2_ & (1u << (index - 64)))) {
      return true;
    } else if (index >= 128 && index < 192 && (map3_ & (1u << (index - 128)))) {
      return true;
    } else if (index >= 192 && (map4_ & (1u << (index - 192)))) {
      return true;
    }
    return false; 
  }

 private:
  uint64_t map1_;
  uint64_t map2_;
  uint64_t map3_;
  uint64_t map4_;
};

#endif  // SCAL_UTIL_BITMAP_H_
