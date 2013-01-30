// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

#include <numeric>

#include "util/atomic_value64_no_offset.h"

namespace {

const uint64_t  kUint64Max = std::numeric_limits<uint64_t>::max();
const AtomicAba kAbaMax = AtomicValue64NoOffset<uint64_t>::kAbaMax;
const uint8_t   kAbaBits = AtomicValue64NoOffset<uint64_t>::kAbaBits;
const uint64_t  kValueMax = AtomicValue64NoOffset<uint64_t>::kValueMax;

}  // namespace

TEST(AtomicValue64NoOffsetTest, EmptyConstructor) {
  AtomicValue64NoOffset<uint64_t> a;
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), 0u);
}

TEST(AtomicValue64NoOffsetTest, ConstructorMax) {
  EXPECT_EQ(kValueMax, kUint64Max >> kAbaBits);
  AtomicValue64NoOffset<uint64_t> a(kValueMax, kAbaMax);
  EXPECT_EQ(a.value(), kValueMax);
  EXPECT_EQ(a.aba(), kAbaMax);
}

TEST(AtomicValue64NoOffsetTest, ConstructorDiff) {
  AtomicValue64NoOffset<uint64_t> a(1234u, 7u);
  EXPECT_EQ(a.value(), 1234u);
  EXPECT_EQ(a.aba(), 7u);
}

TEST(AtomicValue64NoOffsetTest, CAS) {
  AtomicValue64NoOffset<uint64_t> a(1u, 1u);
  AtomicValue64NoOffset<uint64_t> b(2u, 2u);
  EXPECT_EQ(a.value(), 1u);
  EXPECT_EQ(b.value(), 2u);
  EXPECT_FALSE(a.cas(b, b));
  AtomicValue64NoOffset<uint64_t> c(1u, 1u);
  EXPECT_EQ(b.value(), 2u);
  EXPECT_TRUE(a.cas(c, b));
  EXPECT_EQ(a.value(), 2u);
  EXPECT_EQ(a.aba(), 2u);
}

TEST(AtomicValue64NoOffsetTest, Set) {
  AtomicValue64NoOffset<uint64_t> a(1u, 1u);
  a.set_aba(kAbaMax);
  a.set_value(0u);
  EXPECT_EQ(a.value(), 0u);
  EXPECT_EQ(a.aba(), kAbaMax);
  a.set_value(kValueMax);
  a.set_aba(0u);
  EXPECT_EQ(a.aba(), 0u);
  EXPECT_EQ(a.value(), kValueMax);
}

TEST(AtomicValue64NoOffsetTest, CopyCtor) {
  AtomicValue64NoOffset<uint64_t> a(1u, 1u);
  AtomicValue64NoOffset<uint64_t> b = a;
  EXPECT_EQ(a.aba(), 1u);
  EXPECT_EQ(a.value(), 1u);
}

TEST(AtomicValue64NoOffsetTest, AssignmentOperator) {
  AtomicValue64NoOffset<uint64_t> a(1u, 1u);
  AtomicValue64NoOffset<uint64_t> b(0u, 0u);
  b = a;
  EXPECT_EQ(b.aba(), 1u);
  EXPECT_EQ(b.value(), 1u);
}
