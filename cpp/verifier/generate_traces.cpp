#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "operation.h"
#include "trace.h"
#include "semantic.h"

static void run_tests() {
  Operation::test();
  Operations::test();
  Trace::test();
  Traces::test();
}


static Traces* generate_traces(Traces* input, Operations* ops, Semantic* semantic) {
  if (ops->is_single_element()) {
    // Append a single operation to all traces of the input.
    input->appendAll(ops->begin());
    return input;
  }

  Traces* result = new Traces();
  Operations::OverlapIterator* overlaps = ops->Overlaps();

  while (overlaps->has_next()) {
    Operation* op = overlaps->get_next();
    Traces* pruned_input = semantic->prune(input, op);

    if (input->size() == 0 || pruned_input->size() > 0) {
      pruned_input->appendAll(op);

      op->remove();

      Traces* subtraces = generate_traces(pruned_input, ops, semantic);

      result->merge(subtraces);
      if (!ops->is_single_element()) {
        // Subtrace was allocated, except for the last recursion with one operation.
        delete subtraces;
      }

      op->reinsert();
      pruned_input->removeAll(op);
    }
    delete pruned_input;
  }

  // Cleanup iterator.
  delete overlaps;

//  printf("num traces: %d\n", result->size());
  return result;
}

static uint64_t num_traces(int count, Operations* ops) {
  if (ops->is_single_element()) {
    return 1;
  }

  uint64_t result = 0;
  Operations::OverlapIterator* overlaps = ops->Overlaps();

  while (overlaps->has_next()) {
    Operation* op = overlaps->get_next();

    op->remove();

    result += num_traces(count + 1, ops);

    op->reinsert();
  }
  delete overlaps;

  return result;
}


int main(int argc, char** argv) {
  bool count_only = false;
  bool verbose = false;
  int k = 1;
  const char* semantics = NULL;
  int iterations = 1;

  int opt;
  while ((opt = getopt(argc, argv, "ci:k:s:v")) != -1) {
    switch (opt) {
      case 'c': 
        count_only = true;
        break;
      case 'i':
        iterations = atoi(optarg);
        break;
      case 'k':
        k = atoi(optarg);
        break;
      case 's':
        semantics = optarg;
        break;
      case 't':
        run_tests();
        return 0;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        fprintf(stderr, "usage: %s [-cv] [-k num] [-s semantic] filename num_elements\n", argv[0]);
        exit(2);
    }
  }

  if (optind != argc - 2) {
    fprintf(stderr, "usage: %s [-cv] [-k num] [-s semantic] filename num_elements\n", argv[0]);
    exit(2);
  }

  char* filename = argv[optind++];
  int num_ops = atoi(argv[optind++]);

  for (int i = 0; i < iterations; ++i) {
    FILE* input = fopen(filename, "r");
    Operations ops(input, num_ops);
    fclose(input);

    if (verbose) {
      printf("all inserted, consistent %s\n", ops.is_consistent()?"yes":"no");
    }

    if (count_only) {
      printf("traces %" PRIu64 "\n", num_traces(0, &ops));
      return 0;
    }

    Semantic* sem;
    if ((semantics != NULL) && (strcmp(semantics, "fifo") == 0)) {
      sem = new FifoSemantic(k, &ops);
    } else {
      sem = new NullSemantic();
    }
    // Use an empty trace as start.
    Traces base;
    Traces* result = generate_traces(&base, &ops, sem);

    if (verbose) {
      result->print();
    }

    printf("num traces %d\n", result->size());
    delete result;
  }
  return 0;
}
