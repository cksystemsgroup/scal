// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <numeric>

#include "util/atomic_value128.h"

namespace {

const AtomicAba kAbaMax = AtomicValue128<uint64_t>::kAbaMax;
const uint8_t   kAbaBits = AtomicValue128<uint64_t>::kAbaBits;
const uint64_t  kValueMax = AtomicValue128<uint64_t>::kValueMax;

}  // namespace

TEST(AtomicValue128Test, EmptyConstructor) {
  AtomicValue128<uint64_t> a;
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), 0u);
}

TEST(AtomicValue128Test, ConstructorMax) {
  AtomicValue128<uint64_t> a(kValueMax, kAbaMax);
  EXPECT_EQ(a.value(), kValueMax);
  EXPECT_EQ(a.aba(), kAbaMax);
}

TEST(AtomicValue128Test, ConstructorDiff) {
  AtomicValue128<uint64_t> a(1u, kAbaMax - 1);
  EXPECT_EQ(a.value(), 1u);
  EXPECT_EQ(a.aba(), kAbaMax - 1);
}

TEST(AtomicValue128Test, CAS) {
  AtomicValue128<uint64_t> a(1u, 1u);
  AtomicValue128<uint64_t> b(2u, 2u);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue128<uint64_t> c(1u, 1u);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), 2u);
  EXPECT_EQ(a.aba(), 2u);
}

TEST(AtomicValue128Test, WeakSet) {
  AtomicValue128<uint64_t> a(1u, 1u);
  a.weak_set_aba(kAbaMax);
  a.weak_set_value(0u);
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), kAbaMax);
  a.weak_set_value(kValueMax);
  a.weak_set_aba(0u);
  EXPECT_EQ(a.aba(), 0u);
  EXPECT_EQ(a.value(), kValueMax);
}

TEST(AtomicValue128Test, Set) {
  AtomicValue128<uint64_t> a(1u, 1u);
  a.set_aba(kAbaMax);
  a.set_value(0u);
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), kAbaMax);
  a.set_value(kValueMax);
  a.set_aba(0u);
  EXPECT_EQ(a.aba(), 0u);
  EXPECT_EQ(a.value(), kValueMax);
}

TEST(AtomicValue128Test, CopyCtor) {
  AtomicValue128<uint64_t> a(1u, 1u);
  AtomicValue128<uint64_t> b = a;
  EXPECT_EQ(b.aba(), 1u);
  EXPECT_EQ(b.value(), 1u);
}

TEST(AtomicValue128Test, AssignmentOperator) {
  AtomicValue128<uint64_t> a(1u, 1u);
  AtomicValue128<uint64_t> b(0u, 0u);
  b = a;
  EXPECT_EQ(b.aba(), 1u);
  EXPECT_EQ(b.value(), 1u);
}

TEST(AtomicValue128Test, Platform) {
  void *a = malloc(sizeof(a));
  AtomicValue128<void*> av(a, 0);
  EXPECT_EQ(a, av.value());
}
