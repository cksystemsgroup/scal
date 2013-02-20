#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
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
    printf("+ %"PRId64"   %"PRIu64"\n", op->value(), op->real_start());
  } else {
    printf("- %"PRId64"   %"PRIu64"\n", op->value(), op->real_start());
  } 
}

void get_call_frequency(Operation** ops, int num_ops, int factor) {

  uint64_t first_start = UINT64_MAX;
  uint64_t last_start = 0;

  uint64_t min_execution = UINT64_MAX;
  uint64_t max_execution = 0;

  uint64_t total_execution = 0;
  uint64_t tmp = total_execution;

  for (int i = 0; i < num_ops; i++) {

    Operation* op = ops[i];

    if (op->start() < first_start) {
      first_start = op->start();
    }
    if (op->start() > last_start) {
      last_start = op->start();
    }

    uint64_t execution_time = op->end() - op->start();

    if (execution_time < min_execution) {
      min_execution = execution_time;
    }
    if (execution_time > max_execution) {
      max_execution = execution_time;
    }

    total_execution += execution_time;
    assert(total_execution >= tmp);
    tmp = total_execution;
  }

  uint64_t avg_execution = (total_execution) / (uint64_t)num_ops;

  int num_normal = 0;
  uint64_t total_normal = 0;

  for (int i = 0; i < num_ops; i++) {
    Operation* op = ops[i];
    uint64_t execution_time = op->end() - op->start();

    if (execution_time < avg_execution * factor) { 
      num_normal++;
      total_normal += execution_time;
    }
  }

  uint64_t avg_normal = total_normal / (uint64_t)num_normal;
  uint64_t call_freq = (last_start - first_start)/num_ops;

  int num_short = 0;
  int num_long = 0;

  for (int i = 0; i < num_ops; i++) {
    Operation* op = ops[i];
    uint64_t execution_time = op->end() - op->start();

    if (execution_time < avg_normal - call_freq) { 
      num_short++;
    } else if (execution_time > avg_normal + call_freq) {
      num_long++;
    }
  }

//  printf("First start: %"PRIu64"\n", first_start);
//  printf("Last start:  %"PRIu64"\n", last_start);
//  printf("Difference:  %"PRIu64"\n", last_start - first_start);
  printf("Call freq:   %"PRIu64"\n", (last_start - first_start) / (uint64_t)num_ops);
//  printf("Min execution time: %"PRIu64"\n", min_execution);
//  printf("Max execution time:  %"PRIu64"\n", max_execution);
  printf("Avg execution time:          %"PRIu64"\n", (total_execution) / (uint64_t)num_ops);
  printf("Reduced avg execution time:  %"PRIu64"\n", (total_normal) / (uint64_t)num_normal);
  printf("Number of ops:         %d\n", num_ops);
//  printf("Number of normal ops:  %d\n", num_normal);
  printf("Reduced by %.3f%%\n", 100.0 - 100.0*((double)num_normal/(double)num_ops));
  printf("Number of short ops:   %d\n", num_short);
  printf("Number of long ops:    %d\n", num_long);
  printf("Number of normal ops:  %d\n", num_ops - (num_short + num_long));
}

int main(int argc, char** argv) {

  if (argc < 3) {
    fprintf(stderr, "usage: ./linearizer <mode> <operations> <logfilename>\n");
    exit(3);
  }

//  char* mode = argv[1];
  int num_ops = atoi(argv[1]);
  char* filename = argv[2];
  int factor = 5;
  if (argc >= 4) {
    factor = atoi(argv[3]);
  }

  Operation** ops = parse_logfile(filename, num_ops);
  Operation** insert_ops = new Operation*[num_ops];
  Operation** remove_ops = new Operation*[num_ops];

  int num_insert = 0;
  int num_remove = 0;

  for (int i = 0; i < num_ops; i++) {
    
    Operation* op = ops[i];
    if (op->is_insert()) {
      insert_ops[num_insert] = op;
      num_insert++;
    } else {
      remove_ops[num_remove] = op;
      num_remove++;
    }
  }
 
  printf("All operations:\n");
  get_call_frequency(ops, num_ops, factor);
  printf("\n\n\n Insert operations:\n");
  get_call_frequency(insert_ops, num_insert, factor);
  printf("\n\n\n Remove operations:\n");
  get_call_frequency(remove_ops, num_remove, factor);

//  Order** linearization;
//  if (strcmp(mode, "max") == 0) {
//    linearization = linearize_by_min_max(ops, num_ops);
//  } else if (strcmp(mode, "invocation") == 0) {
//    linearization = linearize_by_invocation(ops, num_ops);
//  } else if (strcmp(mode, "lin_point") == 0) {
//    linearization = linearize_by_lin_point(ops, num_ops);
//  } else if (strcmp(mode, "response") == 0) {
//    linearization = linearize_by_response(ops, num_ops);
//  } else if (strcmp(mode, "sum") == 0) {
//    // First we use the lin_point linearization as the initial linearization.
//    linearization = linearize_by_lin_point(ops, num_ops);
//    linearization = linearize_by_min_sum(ops, num_ops, linearization);
//  } else {
//    fprintf(stderr, "The mode %s does not exist, use either 'max', 'invocation', 'lin_point', 'response', or 'sum'\n", mode);
//    exit(11);
//  }
//
//  for (int i = 0; i < num_ops; i++) {
//    assert(linearization[i] != NULL);
//    print_op(linearization[i]->operation);
//  }
  return 0;
}
