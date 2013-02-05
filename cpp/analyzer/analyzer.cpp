#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "analyzer.h"
#include "parser.h"
#include "element.h"

Element** parse_linearization(FILE* input, int num_ops);

int main(int argc, char** argv) {

  if (argc < 2) {
    fprintf(stderr, "usage: .analyzer <operations>\n");
    exit(3);
  }

  int num_ops = atoi(argv[1]);

   Element** elements = parse_linearization(stdin, num_ops);
 
  Result* result = calculate_age(elements, num_ops);

  printf("max: %"PRIu64"; num_ops: %"PRIu64"; total: %"PRIu64"; average: %0.3f\n",
      result->max_costs, result->num_ops, result->total_costs, result->avg_costs);
  
}

