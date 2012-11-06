#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "operation.h"
#include "fifoExecuterLowerBound.h"
#include "fifoExecuterUpperBound.h"
#include "histogram.h"

void enable_frequency_scaling() {
  int result =
      system(
          "for i in  /sys/devices/system/cpu/cpu? /sys/devices/system/cpu/cpu??; do sudo sh -c \"echo ondemand > $i/cpufreq/scaling_governor\"; done");
  if (result) {
    printf("frequency scaling error");
  }
}

void disable_frequency_scaling() {
  int result =
      system(
          "for i in  /sys/devices/system/cpu/cpu? /sys/devices/system/cpu/cpu??; do sudo sh -c \"echo performance > $i/cpufreq/scaling_governor\"; done");
  if (result) {
    printf("frequency scaling error");
  }
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: ./analyzer <logfilename> <operations> <mode>\n");
    return 1;
  }

  char* order = argv[3];
  int operations = atoi(argv[1]);
  char* filename = argv[2];
  int execution_time_quotient = 1;

  if (argc >= 5) {
    execution_time_quotient = atoi(argv[4]);
  }

//  int bound = atoi(argv[4]); //0 lower bound, 1 upper bound
//  char* mode = argv[5];
//  int param = atoi(argv[6]);

//  disable_frequency_scaling();





//  } else {
//    executer = new FifoExecuterUpperBound(&ops);
//  }

  if (strcmp(order, "tool_op") == 0) {

    // Adjust start-times of remove operations because otherwise the tool calculates lin-points for
    // insert-operations which may be before their invocation.
    Operations ops(filename, operations, true);
    ops.shorten_execution_times(execution_time_quotient);
    ops.CalculateOverlaps();
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    Histogram histogram;
    executer->execute(&histogram);
    executer->calculate_order();
    executer->calculate_op_fairness();
  } else if (strcmp(order, "tool") == 0) {
    Operations ops(filename, operations, true);
    if (execution_time_quotient > 1) {
      ops.shorten_execution_times(execution_time_quotient);
    }
    ops.CalculateOverlaps();
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    Histogram histogram;
    executer->execute(&histogram);
    executer->aggregate_semantical_error();
  } else if (strcmp(order, "response") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_response_order();
    executer->calculate_op_fairness_typeless();
  } else if (strcmp(order, "element") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_element_fairness();
  } else if (strcmp(order, "new") == 0) {
    Operations ops(filename, operations, false);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_new_element_fairness(Operation::lin_point_start_time, Operation::compare_operations_by_start_time);
  } else if (strcmp(order, "ef_response") == 0) {
      Operations ops(filename, operations, true);
      FifoExecuter *executer;
      executer = new FifoExecuterLowerBound(&ops);
      executer->calculate_new_element_fairness(Operation::lin_point_end_time, Operation::compare_operations_by_end_time);
  } else if (strcmp(order, "new_sane") == 0) {
    Operations ops(filename, operations, true);
    FifoExecuter *executer;
    executer = new FifoExecuterLowerBound(&ops);
    executer->calculate_new_element_fairness(Operation::lin_point_start_time, Operation::compare_operations_by_start_time);
  } else if (strcmp(order, "perf") == 0) {
      Operations ops(filename, operations, false);
      FifoExecuter *executer;
      executer = new FifoExecuterLowerBound(&ops);
      executer->calculate_performance_index();
  } else {
    printf("Invalid mode, use tool_op, tool, response, element, new, or new_sane, not %s\n", order);
    exit(-1);
  }

//  if (strcmp(datastructure, "fifo") == 0) {
//    if (strcmp(mode, "histogram") == 0) {
//      histogram.print();
//    } else if (strcmp(mode, "sum") == 0) {
//      printf("%" PRIu64 "\n", histogram.commulativeError());
//    } else if (strcmp(mode, "sumparam") == 0) {
//      printf("%d %" PRIu64 " %" PRIu64 " %f %f\n", param, histogram.commulativeError(), histogram.max(), histogram.mean(), histogram.stdv());
//    } else {
//      printf("wrong mode\n");
//    }
//  } else {
//    printf("Fatal Error: datastructure not supported\n");
//    enable_frequency_scaling();
//    exit(2);
//  }
//  enable_frequency_scaling();
  return 0;
}
