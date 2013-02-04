#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include "operation.h"
#include "parser.h"
#include "linearizer.h"
#include <stdlib.h>
#include <time.h>

void print_op(Operation* op) {

  if (op->type() == Operation::INSERT) {
    printf("enqueue %"PRId64"\n", op->value());
  } else {
    printf("              %"PRId64" dequeue\n", op->value());
  } 
}

int main(int argc, char** argv) {

  clock_t start = clock();
  if (argc < 3) {
    fprintf(stderr, "usage: .analyzer <operations> <logfilename>\n");
    exit(3);
  }

  int num_ops = atoi(argv[1]);
  char* filename = argv[2];

  Operation** ops = parse(filename, num_ops);
  
  Order** linearization;
  linearization = linearize_by_min_max(ops, num_ops);
//  linearization = linearize_by_invocation(ops, num_ops);
//  linearization = linearize_by_lin_point(ops, num_ops);
//  linearization = linearize_by_min_sum(ops, num_ops, linearization);
  for (int i = 0; i < 40; i++) {
    print_op(linearization[i]->operation);
  }
  printf("runtime: %lu\n", (clock() - start));

  printf("%d operations.\n", num_ops);
  return 0;
}
