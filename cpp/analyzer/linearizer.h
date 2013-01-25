#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"

#ifndef LINEARIZER_H
#define LINEARIZER_H

struct Order {
  uint64_t order;
  Operation* operation;
};

struct Node {
  Operation* operation;
  Node* matching_op;
  Node* next;
  Node* prev;
  int costs;
};
  
struct Operations {
  // Array of insert operations
  Node** insert_ops;
  // Doubly-linked list of insert operations.
  Node* insert_list;
  // Number of insert operations.
  int num_insert_ops;
  // Array of remove operations.
  Node** remove_ops;
  // Doubly-linked list of remove operations.
  Node* remove_list;
  // Number of remove operations
  int num_remove_ops;
};

Order** linearize(Operation** ops, int num_ops);

#endif // LINEARIZER_H
