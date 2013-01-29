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
  // The operation stored in this node.
  Operation* operation;
  // The operation matching the operation stored in this node, e.g. an insert
  // operation if this node contains a remove operation. The matching operation
  // of a null-remove operation is the remove operation itself.
  Node* matching_op;
  // The next node in the doubly-linked list.
  Node* next;
  // The node which precedes this node in the doubly-linked list.
  Node* prev;
  // The costs of this operation calculated by the cost function.
  int costs;
  // The order in which this node was selected, i.e. removed from the
  // doulby-linked list.
  int order;
  // The first response of any operation in the first overlap group at the time
  // this operation was selected.
  uint64_t latest_lin_point;
  // This flag indicates that the matching insert operations has already been
  // added to the linearization, only necessary for remove operations.
  bool insert_added;
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
