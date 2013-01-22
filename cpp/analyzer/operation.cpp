#include "operation.h"
#include <stdio.h>

Operation::Operation() {
  static uint64_t next_id = 1;
  id_ = next_id++;
  printf("%lu\n", id_);
}

void Operation::initialize(uint64_t start, uint64_t end, OperationType type, uint64_t value) {
}
