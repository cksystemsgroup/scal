#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "element.h"

Operation** parse_logfile(char* filename, int num_ops) {

  FILE* input = fopen(filename, "r");

  Operation** ops = new Operation*[num_ops];

  for (int i = 0; i < num_ops; i++) {
    int type;
    int64_t op_value;
    uint64_t op_start;
    uint64_t op_end;
    uint64_t lin_time;
    Operation::OperationType op_type;

    if (fscanf(input, "%d %"PRId64" %"PRIu64" %"PRIu64" %"PRIu64"\n", 
          &type, &op_value, &op_start, &lin_time, &op_end) == EOF) {
      fprintf(stderr, "ERROR1: could not read all %d elements, abort after %d\n",
          num_ops, i);
      exit(1);
    }

    if (type == 0) {
      op_type = Operation::INSERT;
    } else if (type == 1) {
      op_type = Operation::REMOVE;
    } else {
      fprintf(stderr, "ERROR2: Invalid operation type: %d\n", type);
      exit(2);
    }

    if (op_value == 0) {
      op_value = -1;
    }

    if (lin_time == 0) {
      static int warn_once = 0;
      
      if (warn_once == 0) {
        warn_once++;
        fprintf(stderr, "WARNING: linearization point time stamps are missing\n");
      }
      lin_time = op_end;
    }

    ops[i] = new Operation();
    ops[i]->initialize(op_start, lin_time, op_end, op_type, op_value);
  }

  fclose(input);

  return ops;
}

Element** parse_linearization(FILE* input, int num_ops) {

  Element** result = new Element*[num_ops];

  for (int i = 0; i < num_ops; i++) {
   
    char type;
    Element::ElementType element_type;
    int value;
    uint64_t start;

    if (fscanf(input, "%c %d   %"PRIu64"\n", &type, &value, &start) == EOF) {

      fprintf(stderr, "ERROR11: could not read all %d elements, abort after %d\n",
          num_ops, i);
      exit(11);
    }
    
    if (type == '+') {
      element_type = Element::INSERT;
    } else if (type == '-') {
      element_type = Element::REMOVE;
    } else {
      fprintf(stderr, "ERROR2: Invalid operation type: %d\n", type);
      exit(2);
    }

    result[i] = new Element();
    result[i]->initialize(i, element_type, value);
  }
  return result;
}
