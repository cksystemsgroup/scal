// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <numeric>

#include "util/random.h"
#include "util/threadlocals.h"

namespace {

typedef uint64_t (*random_function1)();
typedef uint64_t (*random_function2)(uint32_t min, uint32_t max);

const uint64_t kClasses = 128;

struct RandomFunctions {
  uint8_t type;
  random_function1 rand1;
  random_function2 rand2;
};

// Perform a chi-square test; alpha: 0.01
bool chi_square_distribution_test(const RandomFunctions &funcs) {
  const uint64_t n = 10000000;
  uint64_t frequency[kClasses];
  const double expected_frequency = static_cast<double>(n) / kClasses;

  for (uint64_t i = 0; i < kClasses; i++) {
    frequency[i] = 0;
  }

  uint64_t rand;
  for (uint64_t i = 0; i < n; i++) {
    switch (funcs.type) {
    case 0:
      rand = funcs.rand1() % kClasses;
      break;
    case 1:
      rand = funcs.rand2(0, kClasses);
      break;
    default:
      abort();
    }
    frequency[rand]++;
  }

  double chi_square = 0;
  for (uint64_t i = 0; i < kClasses; i++) {
    chi_square += ((frequency[i] - expected_frequency) * 
                   (frequency[i] - expected_frequency)) / expected_frequency;
  }

  return chi_square < 135.81;
}

}  // namespace

TEST(PseudoRandomTest, Distribution) {
  threadlocals_init();
  scal::srand((threadlocals_get()->thread_id + 1) * 100);
  RandomFunctions funcs = {0, scal::rand, NULL};
  EXPECT_TRUE(chi_square_distribution_test(funcs));
}

TEST(HwRandomTest, Distribution) {
  threadlocals_init();
  scal::srand((threadlocals_get()->thread_id + 1) * 100);
  RandomFunctions funcs = {0, scal::hwrand, NULL};
  EXPECT_TRUE(chi_square_distribution_test(funcs));
}

TEST(PseudoRandomRangeTest, Distribution) {
  threadlocals_init();
  scal::srand((threadlocals_get()->thread_id + 1) * 100);
  RandomFunctions funcs = {1, NULL, scal::randrange};
  EXPECT_TRUE(chi_square_distribution_test(funcs));
}

