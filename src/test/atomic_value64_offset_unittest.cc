// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

#include <numeric>

#include "util/atomic_value64_offset.h"

namespace {

const uint64_t  kUint64Max = std::numeric_limits<uint64_t>::max();
const AtomicAba kAbaMax = AtomicValue64Offset<uint64_t>::kAbaMax;
const uint8_t   kAbaBits = AtomicValue64Offset<uint64_t>::kAbaBits;
const uint64_t  kValueMax = AtomicValue64Offset<uint64_t>::kValueMax;

}  // namespace

TEST(AtomicValue64OffsetTest, EmptyConstructor) {
  AtomicValue64Offset<uint64_t> a;
  EXPECT_EQ(0u, a.value());
  EXPECT_EQ(0u, a.aba());
}

TEST(AtomicValue64OffsetTest, ConstructorMax) {
  AtomicValue64Offset<uint64_t> a(kValueMax, kAbaMax);
  EXPECT_EQ(kValueMax, a.value());
  EXPECT_EQ(kAbaMax, a.aba());
}

TEST(AtomicValue64OffsetTest, ConstructorDiff) {
  AtomicValue64Offset<uint64_t> a(1234u << 4, 7u);
  EXPECT_EQ(1234u << 4, a.value());
  EXPECT_EQ(7u, a.aba());
}

TEST(AtomicValue64OffsetTest, Set) {
  AtomicValue64Offset<uint64_t> a(1ul, 1ul);
  a.set_aba(kAbaMax);
  a.set_value(0u);
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), kAbaMax);
  a.set_value(kValueMax);
  a.set_aba(0u);
  EXPECT_EQ(a.aba(), 0u);
  EXPECT_EQ(a.value(), kValueMax);
}

TEST(AtomicValue64OffsetTest, CAS) {
  uint64_t val_a = 1u << kAbaBits;
  uint64_t val_b = 2u << kAbaBits;
  AtomicValue64Offset<uint64_t> a(val_a, 1u);
  AtomicValue64Offset<uint64_t> b(val_b, 2u);
  EXPECT_EQ(a.value(), val_a);
  EXPECT_EQ(b.value(), val_b);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue64Offset<uint64_t> c(val_a, 1u);
  EXPECT_EQ(b.value(), val_b);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), val_b);
  EXPECT_EQ(a.aba(), 2u);
}

TEST(AtomicValue64OffsetTest, CopyCtor) {
  AtomicValue64Offset<uint64_t> a(1u << 4, 1u);
  AtomicValue64Offset<uint64_t> b = a;
}

TEST(AtomicValue64OffsetTest, AssignmentOperator) {
  AtomicValue64Offset<uint64_t> a(1u << 4, 1u);
  AtomicValue64Offset<uint64_t> b(0u, 0u);
  b = a;
  EXPECT_EQ(b.aba(), 1u);
  EXPECT_EQ(b.value(), 1u << 4);
}

TEST(AtomicValue64OffsetTest, Platform) {
  void *a = malloc(sizeof(a));
  AtomicValue64Offset<void*> av(a, 0);
  EXPECT_EQ(a, av.value());
}
