
#include <stdio.h>


#include "fifoExecuterLowerBound.h"
#include "fifoExecuterUpperBound.h"
#include "operation.h"
#include "trace.h"

int main(int argc, char** argv) {
  if (argc != 1) {
    fprintf(stderr, "usage: %s\n", argv[0]);
    return 1;
  }

  fprintf(stderr, "start tests ... \n");

//  fprintf(stderr, "test Operation ... \n");
//  Operation::test();
//  fprintf(stderr, "test Operations ... \n");
//  Operations::test();
//  fprintf(stderr, "test Trace ... \n");
//  Trace::test();
//  fprintf(stderr, "test Traces ... \n");
//  Traces::test();
  fprintf(stderr, "FifoExecuterLowerBound test ... \n");
//  FifoExecuterLowerBound::test();
//  fprintf(stderr, "FifoExecuterUpperBound test ... \n");
//  FifoExecuterUpperBound::test();
  fprintf(stderr, "tests finished successfully\n");
  return 0;
}
