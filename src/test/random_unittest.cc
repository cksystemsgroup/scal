// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#include <gtest/gtest.h>
#include <math.h>  // sqrt

#include "util/random.h"
#include "util/threadlocals.h"

namespace {

class RandomWrapper {
 public:
  typedef uint64_t (*random_function1)();
  typedef uint64_t (*random_function2)(uint32_t min, uint32_t max);

  void set_rand1(random_function1 func) {
    rand1_ = func;
    type_ = 0;
  }

  void set_rand2(random_function2 func) {
    rand2_ = func;
    type_ = 1;
  }

  uint64_t rand_range(uint32_t min, uint32_t max) const {
    uint32_t range;
    switch (type_) {
    case 0:
      range = max - min;
      return (rand1_() % range) + min;
    case 1:
      return rand2_(min, max);
    default:
      abort();
    }
  }

 private:
  uint8_t type_;
  random_function1 rand1_;
  random_function2 rand2_;
};

class RandomTest : public testing::Test {
 protected:
  static constexpr uint64_t kN = 1000000;
  static constexpr uint32_t kClasses = 16;
  static constexpr double kChiSquareQuantile = 39.25;  // Depends on kClasses
  static constexpr double kRunTestQuantile = 1.96;

  virtual void SetUp() {
    threadlocals_init();
    scal::srand((threadlocals_get()->thread_id + 1) * 100);
  }

  // Perform a chi-square test; alpha: 0.1%, kClasses-1 degrees of freedom
  bool chi_square(const RandomWrapper &rw) {
    uint64_t frequency[kClasses];
    const double expected_frequency = static_cast<double>(kN) / kClasses;
    for (uint64_t i = 0; i < kClasses; i++) {
      frequency[i] = 0;
    }
    for (uint64_t i = 0; i < kN; i++) {
      uint64_t rand = rw.rand_range(0, kClasses);
      frequency[rand]++;
    }
    double chi_square = 0;
    for (uint64_t i = 0; i < kClasses; i++) {
      chi_square += ((frequency[i] - expected_frequency) * 
                     (frequency[i] - expected_frequency)) / expected_frequency;
    }
    return chi_square < kChiSquareQuantile;
  }

  // Up/Down run test
  bool run_test(const RandomWrapper &rw) {
    const double mu = (2.0 * kN - 3.0) / 3.0;
    const double sigma = sqrt((16.0 * kN - 29.0) / 90.0);

    uint64_t runs = 0;
    uint8_t dir = 0;
    uint64_t prev = rw.rand_range(0, scal::kRandMax);
    for (uint64_t i = 0; i < kN; i++) {
      uint64_t rand = rw.rand_range(0, scal::kRandMax);
      switch (dir) {
      case 1:
        if (rand < prev) {
          dir = 2;
          runs++;
        }
        break;
      case 2:
        if (rand > prev) {
          dir = 1;
          runs++;
        }
        break;
      default:
        if (rand == prev) {
          break;
        } else if (rand > prev) {
          dir = 1;
        } else {  // rand < prev
          dir = 2;
        }
        runs++;
      }
      prev = rand;
    }
    double z = (static_cast<double>(runs) - mu)/sigma;
    return z > -1.96 && z < 1.96;
  }
};

}  // namespace

TEST_F(RandomTest, PseudoRandomDistribution) {
  RandomWrapper rw;
  rw.set_rand1(scal::rand);
  EXPECT_TRUE(chi_square(rw));
}

TEST_F(RandomTest, PseudoRandomRangeDistribution) {
  RandomWrapper rw;
  rw.set_rand2(scal::rand_range);
  EXPECT_TRUE(chi_square(rw));
}

TEST_F(RandomTest, PseudoRandomIndependence) {
  RandomWrapper rw;
  rw.set_rand1(scal::rand);
  EXPECT_TRUE(run_test(rw));
}

TEST_F(RandomTest, HWRandomDistribution) {
  RandomWrapper rw;
  rw.set_rand1(scal::hwrand);
  EXPECT_TRUE(chi_square(rw));
}

// We assume that the hwrand() function does not generate independent random
// values.
TEST_F(RandomTest, HWRandomDependence) {
  RandomWrapper rw;
  rw.set_rand1(scal::hwrand);
  EXPECT_FALSE(run_test(rw));
}
