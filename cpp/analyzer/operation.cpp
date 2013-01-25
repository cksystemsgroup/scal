#include "operation.h"
#include <stdio.h>

Operation::Operation() {
  static uint64_t next_id = 1;
  id_ = next_id++;
}

void Operation::initialize(uint64_t start, uint64_t lin_time, uint64_t end, OperationType type, uint64_t value) {
  start_ = start;
  lin_time_ = lin_time;
  end_ = end;
  type_ = type;
  value_ = value;
}
