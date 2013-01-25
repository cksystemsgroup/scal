#include <stdio.h>
#include "operation.h"
#include "parser.h"
#include "linearizer.h"
#include <stdlib.h>

int main(int argc, char** argv) {

  if (argc < 3) {
    fprintf(stderr, "usage: .analyzer <operations> <logfilename>\n");
    exit(3);
  }

  int num_ops = atoi(argv[1]);
  char* filename = argv[2];

  Operation** ops = parse(filename, num_ops);
  
  linearize(ops, num_ops);
  

  printf("%d operations.\n", num_ops);
}
