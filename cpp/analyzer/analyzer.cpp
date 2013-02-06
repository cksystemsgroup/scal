#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "analyzer.h"
#include "parser.h"
#include "element.h"

int main(int argc, char** argv) {

  if (argc < 2) {
    fprintf(stderr, "usage: .analyzer <operations>\n");
    exit(3);
  }

  int num_ops = atoi(argv[1]);

  FILE* input = stdin;

  if (argc >= 3) {
    input = fopen(argv[2], "r");
  }
  
  Element** elements = parse_linearization(input, num_ops);
 
  if (input != stdin) {
    fclose(input);
  }

  Result* result = calculate_age(elements, num_ops);

  printf("max: %"PRIu64"; num_ops: %"PRIu64"; total: %"PRIu64"; average: %0.3f\n",
      result->max_costs, result->num_ops, result->total_costs, result->avg_costs);
  
}

