#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include "operation.h"
#include "parser.h"
#include "linearizer.h"
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

void print_op(Operation* op) {

  assert(op != NULL);
  if (op->type() == Operation::INSERT) {
    printf("+ %"PRId64"   %"PRIu64"\n", op->value(), op->start());
  } else {
    printf("- %"PRId64"   %"PRIu64"\n", op->value(), op->start());
  } 
}

void test_selectable_remove() {
  Operation i1, i2, i3, i4, r1, r2, r3, r4;

  const int num_ops = 8;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &i2
    , &r2
    , &i3
    , &r3
    , &i4
    , &r4
  };

  i1.initialize( 1, 0,  2, Operation::INSERT, 1);
  i2.initialize( 3, 0,  4, Operation::INSERT, 2);
  i3.initialize( 5, 0,  6, Operation::INSERT, 3);
  i4.initialize( 7, 0,  8, Operation::INSERT, 4);
  r4.initialize( 9, 0, 11, Operation::REMOVE, 4);
  r3.initialize(10, 0, 16, Operation::REMOVE, 3);
  r1.initialize(12, 0, 13, Operation::REMOVE, 1);
  r2.initialize(14, 0, 15, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_sum(ops, num_ops, linearize_by_invocation(ops, num_ops));
  
  assert (linearization[0]->operation == &i1);
  assert (linearization[1]->operation == &i2);
  assert (linearization[2]->operation == &i3);
  assert (linearization[3]->operation == &i4);
  assert (linearization[4]->operation == &r4);
  assert (linearization[5]->operation == &r1);
  assert (linearization[6]->operation == &r2);
  assert (linearization[7]->operation == &r3);
  printf("%s passed!\n", __func__);
}

void test_selectable_insert() {
  Operation i1, i2, r1, r2, n1;

  const int num_ops = 5;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
  };

  i1.initialize( 1, 0,  9, Operation::INSERT, 1);
  i2.initialize( 2, 0,  3, Operation::INSERT, 2);
  n1.initialize( 8, 0, 10, Operation::REMOVE, -1);
  r1.initialize( 4, 0,  5, Operation::REMOVE, 1);
  r2.initialize( 6, 0,  7, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_sum(ops, num_ops, linearize_by_invocation(ops, num_ops));
  
  assert (linearization[0]->operation == &i1);
  assert (linearization[1]->operation == &i2);
  assert (linearization[2]->operation == &r1);
  assert (linearization[3]->operation == &r2);
  assert (linearization[4]->operation == &n1);
  printf("%s passed!\n", __func__);
}


void test_equal_time_stamp_max() {
  Operation i1, i2, r1, r2, n1;

  const int num_ops = 5;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
  };

  i1.initialize( 1, 0,  2, Operation::INSERT, 1);
  n1.initialize( 2, 0,  4, Operation::REMOVE, -1);
  r1.initialize( 3, 0,  5, Operation::REMOVE, 1);
  i2.initialize( 6, 0,  7, Operation::INSERT, 2);
  r2.initialize( 8, 0,  9, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_max(ops, num_ops);
  
  assert (linearization[0]->operation == &n1);
  assert (linearization[1]->operation == &i1);
  assert (linearization[2]->operation == &r1);
  assert (linearization[3]->operation == &i2);
  assert (linearization[4]->operation == &r2);
  printf("%s passed!\n", __func__);
}

void test_equal_time_stamp_sum() {
  Operation i1, i2, r1, r2, n1;

  const int num_ops = 5;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
  };

  i1.initialize( 1, 0,  2, Operation::INSERT, 1);
  n1.initialize( 2, 0,  4, Operation::REMOVE, -1);
  r1.initialize( 3, 0,  5, Operation::REMOVE, 1);
  i2.initialize( 6, 0,  7, Operation::INSERT, 2);
  r2.initialize( 8, 0,  9, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_sum(ops, num_ops, linearize_by_invocation(ops, num_ops));
  
  assert (linearization[0]->operation == &n1);
  assert (linearization[1]->operation == &i1);
  assert (linearization[2]->operation == &r1);
  assert (linearization[3]->operation == &i2);
  assert (linearization[4]->operation == &r2);
  printf("%s passed!\n", __func__);
}

void test_null_return1() {
  Operation i1, i2, r1, r2, n1;

  const int num_ops = 5;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
  };

  i1.initialize(1, 0, 2, Operation::INSERT, 1);
  n1.initialize(3, 0, 10, Operation::REMOVE, -1);
  i2.initialize(4, 0, 5, Operation::INSERT, 2);
  r1.initialize(6, 0, 7, Operation::REMOVE, 1);
  r2.initialize(8, 0, 9, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_max(ops, num_ops);
  
  assert (linearization[0]->operation == &i1);
  assert (linearization[1]->operation == &i2);
  assert (linearization[2]->operation == &r1);
  assert (linearization[3]->operation == &r2);
  assert (linearization[4]->operation == &n1);
  printf("%s passed!\n", __func__);
}

void test_null_return2() {
  Operation i1, i2, i3, r1, r2, r3, n1;

  const int num_ops = 7;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
    , &i3
    , &r3
  };

  r1.initialize( 1, 0, 14, Operation::REMOVE, 1);
  i2.initialize( 2, 0, 13, Operation::INSERT, 2);
  n1.initialize( 3, 0, 10, Operation::REMOVE, -1);
  i3.initialize( 4, 0, 5 , Operation::INSERT, 3);
  i1.initialize( 6, 0, 7 , Operation::INSERT, 1);
  r2.initialize( 8, 0, 9 , Operation::REMOVE, 2);
  r3.initialize(11, 0, 12, Operation::REMOVE, 3);

  Order** linearization;
  linearization = linearize_by_min_max(ops, num_ops);
  
  assert (linearization[0]->operation == &n1);
  assert (linearization[1]->operation == &i2);
  assert (linearization[2]->operation == &i3);
  assert (linearization[3]->operation == &i1);
  assert (linearization[4]->operation == &r2);
  assert (linearization[5]->operation == &r3);
  assert (linearization[6]->operation == &r1);
  printf("%s passed!\n", __func__);
}

void test_null_return3() {
  Operation i1, i2, i3, i4, i5, r1, r2, r3, r4, r5, n1;

  const int num_ops = 11;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
    , &i3
    , &r3
    , &i4
    , &r4
    , &i5
    , &r5
  };

  i4.initialize(  2, 0,   4, Operation::INSERT, 4);
  i5.initialize(  6, 0,   8, Operation::INSERT, 5);
  i2.initialize( 20, 0, 130, Operation::INSERT, 2);
  r1.initialize( 10, 0, 140, Operation::REMOVE, 1);
  i2.initialize( 20, 0, 130, Operation::INSERT, 2);
  r4.initialize( 92, 0,  94, Operation::REMOVE, 4);
  r5.initialize( 96, 0,  98, Operation::REMOVE, 5);
  n1.initialize( 30, 0, 100, Operation::REMOVE, -1);
  i3.initialize( 40, 0,  50, Operation::INSERT, 3);
  i1.initialize( 60, 0,  70, Operation::INSERT, 1);
  r2.initialize( 80, 0,  90, Operation::REMOVE, 2);
  r3.initialize(110, 0, 120, Operation::REMOVE, 3);

  Order** linearization;
  linearization = linearize_by_min_max(ops, num_ops);

//  for (int i = 0; i < num_ops; i++) {
//    assert(linearization[i] != NULL);
//    print_op(linearization[i]->operation);
//  }
  
  assert (linearization[0]->operation == &i4);
  assert (linearization[1]->operation == &i5);
  assert (linearization[2]->operation == &n1);
  assert (linearization[3]->operation == &i2);
  assert (linearization[4]->operation == &i3);
  assert (linearization[5]->operation == &i1);
  assert (linearization[6]->operation == &r2);
  assert (linearization[7]->operation == &r4);
  assert (linearization[8]->operation == &r5);
  assert (linearization[9]->operation == &r3);
  assert (linearization[10]->operation == &r1);
  printf("%s passed!\n", __func__);
}

/// This test tests the selectable function for null-returns
void test_null_return4() {
  Operation i1, i2, i3, r1, r2, r3, n1;

  const int num_ops = 7;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &i2
    , &r2
    , &i3
    , &r3
  };

  i1.initialize( 1, 0,  2, Operation::INSERT, 1);
  i2.initialize( 3, 0,  5, Operation::INSERT, 2);
  n1.initialize( 4, 0, 14, Operation::REMOVE, -1);
  i3.initialize( 6, 0,  7, Operation::INSERT, 3);
  r3.initialize( 8, 0,  9, Operation::REMOVE, 3);
  r1.initialize(10, 0, 11, Operation::REMOVE, 1);
  r2.initialize(12, 0, 13, Operation::REMOVE, 2);

  Order** linearization;
  linearization = linearize_by_min_sum(ops, num_ops, linearize_by_invocation(ops, num_ops));
  
  assert (linearization[0]->operation == &i1);
  assert (linearization[1]->operation == &i2);
  assert (linearization[2]->operation == &i3);
  assert (linearization[3]->operation == &r3);
  assert (linearization[4]->operation == &r1);
  assert (linearization[5]->operation == &r2);
  assert (linearization[6]->operation == &n1);
  printf("%s passed!\n", __func__);
}

// This test checks if the selectable function for insert operations considers
// null-returns.
void test_null_return5() {
  Operation i1, i2, r1, r2, n1, n2;

  const int num_ops = 6;
  Operation* ops[num_ops] = {
      &i1
    , &r1
    , &n1
    , &n2
    , &i2
    , &r2
  };

  i1.initialize( 1, 0,  3, Operation::INSERT, 1);
  i2.initialize( 2, 0,  8, Operation::INSERT, 2);
  n1.initialize( 4, 0,  5, Operation::REMOVE, -1);
  n2.initialize( 6, 0,  7, Operation::REMOVE, -1);
  r2.initialize( 9, 0, 10, Operation::REMOVE, 2);
  r1.initialize(11, 0, 12, Operation::REMOVE, 1);

  Order** linearization;
  linearization = linearize_by_min_sum(ops, num_ops, linearize_by_invocation(ops, num_ops));
  
  assert (linearization[0]->operation == &i1);
  assert (linearization[1]->operation == &n1);
  assert (linearization[2]->operation == &n2);
  assert (linearization[3]->operation == &i2);
  assert (linearization[4]->operation == &r2);
  assert (linearization[5]->operation == &r1);
  printf("%s passed!\n", __func__);
}

int main(int argc, char** argv) {

  test_selectable_remove();
  test_selectable_insert();
  test_equal_time_stamp_max();
  test_equal_time_stamp_sum();
  test_null_return1();
  test_null_return2();
  test_null_return3();
  test_null_return5();
  test_null_return4();
}

