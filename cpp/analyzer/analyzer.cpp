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
    printf("        %"PRId64" dequeue\n", op->value());
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
  
  Order** linearization = linearize(ops, num_ops);

  for (int i = 0; i < 50; i++) {
    print_op(linearization[i]->operation);
  }
  printf("runtime: %lu\n", (clock() - start));

  printf("%d operations.\n", num_ops);
  return 0;
}
