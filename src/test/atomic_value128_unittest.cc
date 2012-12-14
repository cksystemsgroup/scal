// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <numeric>

#include "gtest/gtest.h"
#include "util/atomic_value128.h"

TEST(AtomicValue128Test, EmptyConstructor) {
  AtomicValue128<uint64_t> a;
  EXPECT_EQ(0ul, a.value());
  EXPECT_EQ(0ul, a.aba());
}

TEST(AtomicValue128Test, ConstructorMax) {
  AtomicValue128<uint64_t> a(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.value());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.aba());
}

TEST(AtomicValue128Test, ConstructorDiff) {
  AtomicValue128<uint64_t> a(1ul, std::numeric_limits<uint64_t>::max()-1);
  EXPECT_EQ(1ul, a.value());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max() - 1, a.aba());
}

TEST(AtomicValue128Test, CAS) {
  AtomicValue128<uint64_t> a(1ul, 1ul);
  AtomicValue128<uint64_t> b(2ul, 2ul);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue128<uint64_t> c(1ul, 1ul);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), 2ul);
  EXPECT_EQ(a.aba(), 2ul);
}

TEST(AtomicValue128Test, WeakSet) {
  AtomicValue128<uint64_t> a(1UL, 1UL);
  a.weak_set_aba(std::numeric_limits<uint64_t>::max());
  a.weak_set_value(0ul);
  EXPECT_EQ(0ul, a.value());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.aba());
  a.weak_set_value(std::numeric_limits<uint64_t>::max());
  a.weak_set_aba(0ul);
  EXPECT_EQ(0UL, a.aba());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.value());
}

TEST(AtomicValue128Test, Set) {
  AtomicValue128<uint64_t> a(1UL, 1UL);
  a.set_aba(std::numeric_limits<uint64_t>::max());
  a.set_value(0ul);
  EXPECT_EQ(0UL, a.value());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.aba());
  a.set_value(std::numeric_limits<uint64_t>::max());
  a.set_aba(0ul);
  EXPECT_EQ(0UL, a.aba());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), a.value());
}

TEST(AtomicValue128Test, CopyCtor) {
  AtomicValue128<uint64_t> a(1ul, 1UL);
  AtomicValue128<uint64_t> b = a;
  EXPECT_EQ(1UL, b.aba());
  EXPECT_EQ(1UL, b.value());
}

TEST(AtomicValue128Test, AssignmentOperator) {
  AtomicValue128<uint64_t> a(1ul, 1ul);
  AtomicValue128<uint64_t> b(0ul, 0ul);
  b = a;
  EXPECT_EQ(1ul, b.aba());
  EXPECT_EQ(1ul, b.value());
}

TEST(AtomicValue128Test, Platform) {
  void *a = malloc(sizeof(void*));
  AtomicValue128<void*> av(a, 0);
  EXPECT_EQ(a, av.value());
}
