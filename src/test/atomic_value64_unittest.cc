// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

#include <numeric>

#include "gtest/gtest.h"
#include "util/atomic_value64_offset.h"
#include "util/atomic_value64_no_offset.h"

TEST(AtomicValue64OffsetTest, EmptyConstructor) {
  AtomicValue64Offset<uint64_t> a;
  EXPECT_EQ(0ul, a.value());
  EXPECT_EQ(0ul, a.aba());
}

TEST(AtomicValue64OffsetTest, ConstructorMax) {
  uint8_t aba_max = 15;
  AtomicValue64Offset<uint64_t> a(std::numeric_limits<uint64_t>::max() - aba_max, aba_max);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max() - aba_max, a.value());
  EXPECT_EQ(aba_max, a.aba());
}

TEST(AtomicValue64OffsetTest, ConstructorDiff) {
  AtomicValue64Offset<uint64_t> a(1234u << 4, 7u);
  EXPECT_EQ(1234u << 4, a.value());
  EXPECT_EQ(7u, a.aba());
}

TEST(AtomicValue64OffsetTest, CAS) {
  uint8_t aba_bits = 4u;
  AtomicValue64Offset<uint64_t> a(1u << aba_bits, 1u);
  AtomicValue64Offset<uint64_t> b(2u << aba_bits, 2u);
  EXPECT_EQ(a.value(), 1u << aba_bits);
  EXPECT_EQ(b.value(), 2u << aba_bits);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue64Offset<uint64_t> c(1u << aba_bits, 1u);
  EXPECT_EQ(b.value(), 2u << aba_bits);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), 2u << aba_bits);
  EXPECT_EQ(a.aba(), 2u);
}

TEST(AtomicValue64OffsetTest, Set) {
  uint8_t aba_max = 15;
  uint8_t aba_bits = 4;
  AtomicValue64Offset<uint64_t> a(1ul, 1ul);
  a.set_aba(aba_max);
  a.set_value(0ul);
  EXPECT_EQ(0ul, a.value());
  EXPECT_EQ(aba_max, a.aba());
  a.set_value(std::numeric_limits<uint64_t>::max() << aba_bits);
  a.set_aba(0u);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max() << aba_bits, a.value());
  EXPECT_EQ(0u, a.aba());
}

TEST(AtomicValue64OffsetTest, CopyCtor) {
  AtomicValue64Offset<uint64_t> a(1ul << 4, 1ul);
  AtomicValue64Offset<uint64_t> b = a;
  EXPECT_EQ(1ul, b.aba());
  EXPECT_EQ(1ul << 4, b.value());
}

TEST(AtomicValue64OffsetTest, AssignmentOperator) {
  AtomicValue64Offset<uint64_t> a(1ul << 4, 1ul);
  AtomicValue64Offset<uint64_t> b(0ul, 0ul);
  b = a;
  EXPECT_EQ(1ul, b.aba());
  EXPECT_EQ(1ul << 4, b.value());
}

TEST(AtomicValue64OffsetTest, Platform) {
  void *a = malloc(sizeof(void*));
  AtomicValue64Offset<void*> av(a, 0);
  EXPECT_EQ(a, av.value());
}

TEST(AtomicValue64NoOffsetTest, EmptyConstructor) {
  AtomicValue64NoOffset<uint64_t> a;
  EXPECT_EQ(0UL, a.value());
  EXPECT_EQ(0UL, a.aba());
}

TEST(AtomicValue64NoOffsetTest, ConstructorMax) {
  uint8_t aba_max = 15;
  uint8_t aba_bits = 4;
  AtomicValue64NoOffset<uint64_t> a(std::numeric_limits<uint64_t>::max() >> aba_bits, aba_max);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max() >> aba_bits, a.value());
  EXPECT_EQ(aba_max, a.aba());
}

TEST(AtomicValue64NoOffsetTest, ConstructorDiff) {
  AtomicValue64NoOffset<uint64_t> a(1234u, 7u);
  EXPECT_EQ(1234u, a.value());
  EXPECT_EQ(7u, a.aba());
}

TEST(AtomicValue64NoOffsetTest, CAS) {
  uint8_t aba_max = 15U;
  AtomicValue64NoOffset<uint64_t> a(aba_max + 1U, 1U);
  AtomicValue64NoOffset<uint64_t> b(aba_max + 2U, 2U);
  EXPECT_EQ(a.value(), aba_max + 1U);
  EXPECT_EQ(b.value(), aba_max + 2U);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue64NoOffset<uint64_t> c(aba_max + 1, 1U);
  EXPECT_EQ(b.value(), aba_max + 2U);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), aba_max + 2U);
  EXPECT_EQ(a.aba(), 2U);
}

TEST(AtomicValue64NoOffsetTest, Set) {
  AtomicValue64NoOffset<uint64_t> a(1UL, 1U);
  uint8_t aba_max = 15;
  uint8_t aba_bits = 4;
  a.set_aba(aba_max);
  a.set_value(0ul);
  EXPECT_EQ(0UL, a.value());
  EXPECT_EQ(aba_max, a.aba());
  a.set_value(std::numeric_limits<uint64_t>::max() >> aba_bits);
  a.set_aba(0u);
  EXPECT_EQ(0u, a.aba());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max() >> aba_bits, a.value());
}

TEST(AtomicValue64NoOffsetTest, CopyCtor) {
  AtomicValue64NoOffset<uint64_t> a(1ul, 1u);
  AtomicValue64NoOffset<uint64_t> b = a;
  EXPECT_EQ(1U, b.aba());
  EXPECT_EQ(1U, b.value());
}

TEST(AtomicValue64NoOffsetTest, AssignmentOperator) {
  AtomicValue64NoOffset<uint64_t> a(1UL, 1U);
  AtomicValue64NoOffset<uint64_t> b(0UL, 0U);
  b = a;
  EXPECT_EQ(1U, b.aba());
  EXPECT_EQ(1UL, b.value());
}
