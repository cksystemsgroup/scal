#include "operation.h"
#include <stdio.h>

Operation::Operation() {
  static uint64_t next_id = 1;
  id_ = next_id++;
}

void Operation::initialize(uint64_t start, uint64_t lin_time, uint64_t end, OperationType type, uint64_t value) {
  start_ = start;
  real_start_ = start;
  lin_time_ = lin_time;
  end_ = end;
  real_end_ = end;
  type_ = type;
  value_ = value;
}

void Operation::adjust_start(uint64_t new_start) {
  start_ = new_start;
}

void Operation::adjust_end(uint64_t new_end) {
  end_ = new_end;
}
