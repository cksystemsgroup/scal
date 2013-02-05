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
    printf("+ %"PRId64"\n", op->value());
  } else {
    printf("- %"PRId64"\n", op->value());
  } 
}

int main(int argc, char** argv) {

  if (argc < 4) {
    fprintf(stderr, "usage: ./linearizer <mode> <operations> <logfilename>\n");
    exit(3);
  }

  char* mode = argv[1];
  int num_ops = atoi(argv[2]);
  char* filename = argv[3];

  Operation** ops = parse_logfile(filename, num_ops);
  
  Order** linearization;

  if (strcmp(mode, "max") == 0) {
    linearization = linearize_by_min_max(ops, num_ops);
  } else if (strcmp(mode, "invocation") == 0) {
    linearization = linearize_by_invocation(ops, num_ops);
  } else if (strcmp(mode, "lin_point") == 0) {
    linearization = linearize_by_lin_point(ops, num_ops);
  } else if (strcmp(mode, "response") == 0) {
    linearization = linearize_by_response(ops, num_ops);
  } else if (strcmp(mode, "sum") == 0) {
    // First we use the lin_point linearization as the initial linearization.
    linearization = linearize_by_lin_point(ops, num_ops);
    linearization = linearize_by_min_sum(ops, num_ops, linearization);
  } else {
    fprintf(stderr, "The mode %s does not exist, use either 'max', 'invocation', 'lin_point', 'response', or 'sum'\n", mode);
    exit(11);
  }

  for (int i = 0; i < num_ops; i++) {
    assert(linearization[i] != NULL);
    print_op(linearization[i]->operation);
  }
  return 0;
}
