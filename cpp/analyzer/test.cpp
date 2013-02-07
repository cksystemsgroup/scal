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

void test_null_return_start_time() {
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
  printf("test_null_return_start_time() passed!\n");
}

int main(int argc, char** argv) {

  test_null_return_start_time();
}

