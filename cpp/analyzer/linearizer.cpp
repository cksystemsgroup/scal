#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "linearizer.h"
#include <limits.h>

void match_operations(Operations* operations);
void create_insert_and_remove_array(Operations* operations, Operation** ops, int num_ops);
void create_doubly_linked_lists(Node* head, Node** ops, int num_ops);
void initialize(Operations* operations, Operation** ops, int num_ops);
Node* linearize_remove_ops(Operations* operations);
Node* linearize_insert_ops(Operations* operations);
void next_order(Order** order, Operation* operation, uint64_t* next_index);

int compare_operations_by_start_time(const void* left, const void* right) {
  return (*(Node**) left)->operation->start() > (*(Node**) right)->operation->start();
}

int compare_operations_by_value(const void* left, const void* right) {
  return (*(Node**) left)->operation->value() > (*(Node**) right)->operation->value();
}

void remove_from_list(Node* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = NULL;
  node->prev = NULL;
}

void insert_to_list(Node* node, Node* predecessor) {
  node->prev = predecessor;
  node->next = predecessor->next;
  predecessor->next->prev = node;
  predecessor->next = node;
}

Order** merge_linearizations(Node* insert_linearization, Node* remove_linearization, int num_ops) {

  Order** result = new Order*[num_ops];
  uint64_t next_index = 0;

  Node* next_insert = insert_linearization->next;
  Node* next_remove = remove_linearization->next;

  while (next_insert != insert_linearization) {

    if (next_remove != remove_linearization && next_remove->insert_added &&
        next_remove->operation->start() < next_insert->latest_lin_point) {

      next_order(result, next_remove->operation, &next_index);
      next_remove = next_remove->next;

    } else {
      next_order(result, next_insert->operation, &next_index);
      next_insert->matching_op->insert_added = true;
      next_insert = next_insert->next;
    }
  }

  return result;
}

Order** linearize(Operation** ops, int num_ops) {

  Operations* operations = new Operations();

  initialize(operations, ops, num_ops);

  Node* remove_linearization = linearize_remove_ops(operations);
  Node* insert_linearization = linearize_insert_ops(operations);

  printf("linearizations finished\n");

  return merge_linearizations(insert_linearization, remove_linearization, num_ops);

}

int get_insert_costs(Operations* operations, Node* insert_op) {

  assert (insert_op != NULL);
  assert (insert_op->matching_op != NULL);

  int costs = insert_op->matching_op->order;
  Node* next_op = insert_op->next;
  while (next_op != operations->insert_list &&
      next_op->operation->start() <= insert_op->operation->end()) {

    if (next_op->matching_op->order < costs) {
      costs = next_op->matching_op->order;
    }
    next_op = next_op->next;
  }

  insert_op->costs = costs;
  return costs;
}

void next_order(Order** order, Operation* operation, uint64_t* next_index) {

  Order* new_order = new Order();
  new_order->operation = operation;
  new_order->order = *next_index;
  order[*next_index] = new_order;
  (*next_index)++;
}

Node* linearize_insert_ops(Operations* operations) {

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list,
      operations->insert_ops, operations->num_insert_ops);

  Node* insert_linearization = new Node();
  insert_linearization->next = insert_linearization;
  insert_linearization->prev = insert_linearization;
  Node* last_selected = NULL;

  int next_order = 0;

  while (operations->insert_list->next != operations->insert_list) {

    Node* op = operations->insert_list->next;
    uint64_t first_end = op->operation->end();

    Node* maximal_costs_op = op;
    int maximal_costs = -1;

    Node* minimal_order_op = op;
    int minimal_order = INT_MAX;

    // Selectable bound: operations which respond after the selectable bound
    // cannot be selected.
 //   uint64_t selectable_bound = (uint64_t)-1;

    // There is no need to calculate the costs of operations which respond
    // before the last_end because its costs cannot be lower than the costs of
    // already calculated operations.
//    uint64_t last_end = op->operation->end;

    // Only operations which start before the first remove operation ends are
    // within the first overlap group.
    while (op != operations->insert_list && op->operation->start() <= first_end) {

      if (op->operation->end() < first_end) {
        first_end = op->operation->end();
      }

      int costs = get_insert_costs(operations, op);

      if (costs > maximal_costs) {

        maximal_costs = costs;
        maximal_costs_op = op;
      } else if (costs == maximal_costs &&
          op->matching_op->order < maximal_costs_op->matching_op->order) {

        maximal_costs_op = op;
      }

      if (op->matching_op->order < minimal_order) {
        minimal_order = op->matching_op->order;
        minimal_order_op = op;
      }

      op = op->next;
    }

    if (minimal_order < maximal_costs) {
      maximal_costs_op = minimal_order_op;
    }

    remove_from_list(maximal_costs_op);
    maximal_costs_op->order = next_order;
    next_order++;
    maximal_costs_op->latest_lin_point = first_end;

    if (last_selected == NULL) {
      insert_to_list(maximal_costs_op, insert_linearization);
    } else {
      insert_to_list(maximal_costs_op, last_selected);
    }

    last_selected = maximal_costs_op;

  }
  delete(operations->insert_list);
  operations->insert_list = NULL;

  return insert_linearization;
}

///
// This function calculates the costs of remove operations.
//
// operations All operations.
// remove_op The remove operation the costs are calculated for.
// last_selected_op The operation which was selected last by the linearization
//                 algorithm. Used for the optimization.
// optimize A flag which turns on the optimization.
int get_remove_costs(Operations* operations, Node* remove_op, Node* last_selected_op, bool optimize) {

  // If we have calculated the costs already once, we can reuse the result. If
  // the insert operation matching the last_selected_op strictly precedes the
  // insert operation matching the remove_op, then the costs decrease by 1,
  // otherwise they remain the same. Note that this is just an optimization.
  if (optimize && remove_op->costs >= 0) {
    if (last_selected_op != NULL) {
      if (last_selected_op->matching_op->operation->end() <
          remove_op->matching_op->operation->start() &&
          last_selected_op->operation->value() != -1) {
        remove_op->costs--;
        if (remove_op->costs < 0) {
          printf("Start %"PRIu64" < %"PRIu64" End\n",
              remove_op->matching_op->operation->start(),
              last_selected_op->matching_op->operation->end());
          exit(5);
        }
      }
    }
  }
  else {

    Node* matching_insert = remove_op->matching_op;
    remove_op->costs = 0;
    Node* next_op = operations->insert_list->next;
    while (next_op != operations->insert_list) {

      // No further insert operations can be found which strictly precede the
      // matching insert operation.
      if (next_op->operation->start() > matching_insert->operation->start()) {
        break;
      }
      // The operation op strictly precedes matching_insert. Costs increase by
      // one.
      if (next_op->operation->end() < matching_insert->operation->start()) {
        remove_op->costs++;
      }

      next_op = next_op->next;
    }
  }
  assert(remove_op->costs >= 0);
  return remove_op->costs;
}

Node* linearize_remove_ops(Operations* operations) {

  int x = 0;
  int64_t total_costs = 0;
  Node* last_selected = NULL;
  // The resulting remove linearization.
  Node* remove_linearization = new Node();
  // Initialize an empty doubly-linked list.
  remove_linearization->next = remove_linearization;
  remove_linearization->prev = remove_linearization;

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list,
      operations->insert_ops, operations->num_insert_ops);

  operations->remove_list = new Node();
  operations->remove_list->operation = NULL;
  create_doubly_linked_lists(operations->remove_list,
      operations->remove_ops, operations->num_remove_ops);

  int next_order = 0;

  while (operations->remove_list->next != operations->remove_list) {

    Node* op = operations->remove_list->next;
    uint64_t first_end = op->operation->end();

    Node* minimal_costs_op = op;

    int minimal_costs = INT_MAX;

    // Only operations which start before the first remove operation ends are
    // within the first overlap group.
    while (op != operations->remove_list && op->operation->start() <= first_end) {

      if (op->operation->end() < first_end) {
        first_end = op->operation->end();
      }

      int costs = get_remove_costs(operations, op, last_selected, true);
      if (costs < minimal_costs) {

        minimal_costs = costs;
        minimal_costs_op = op;
      } else if (costs == minimal_costs &&
          op->matching_op->operation->start() <
          minimal_costs_op->matching_op->operation->start()) {

        minimal_costs_op = op;
      }
      op = op->next;
    }

    // Remove the operation with minimal costs from the list of unselected
    // operations.
    remove_from_list(minimal_costs_op);

    // Assign the order index to the removed operation.
    minimal_costs_op-> order = next_order;
    next_order++;
    // The first response of any operation in the first overlap group is the
    // last possible linearization point of the operation.
    minimal_costs_op->latest_lin_point = first_end;

    // Remove the matching insert operation from its list, but only if the
    // minimal_cost_op is not a null-return remove operation.
    if (minimal_costs_op != minimal_costs_op->matching_op) {
      remove_from_list(minimal_costs_op->matching_op);
    }

    if (last_selected == NULL) {
      insert_to_list(minimal_costs_op, remove_linearization);
    } else {
      insert_to_list(minimal_costs_op, last_selected);
    }

    last_selected = minimal_costs_op;
    x++;
    total_costs += minimal_costs;

  }

  delete(operations->remove_list);
  operations->remove_list = NULL;
  delete(operations->insert_list);
  operations->insert_list = NULL;

  printf("After list iteration x=%d with %"PRId64" costs\n", x, total_costs);
  return remove_linearization;
}

void initialize(Operations* operations, Operation** ops, int num_ops) {

  create_insert_and_remove_array(operations, ops, num_ops);
  match_operations(operations);
}

void match_operations(Operations* operations) {

  qsort(operations->insert_ops, operations->num_insert_ops, sizeof(Node*),
      compare_operations_by_value);

  qsort(operations->remove_ops, operations->num_remove_ops, sizeof(Node*),
      compare_operations_by_value);

  int insert_index = 0;

  int i = 0;
  printf("Number of remove ops: %d\n", operations->num_remove_ops);
  for (int remove_index = 0; remove_index < operations->num_remove_ops; remove_index++) {

    int64_t value = operations->remove_ops[remove_index]->operation->value();

    // If the remove operation is not a null-return.
    if (value != -1) {
      while (operations->insert_ops[insert_index]->operation->value() < value) {
        printf("insert without remove: %"PRIu64" because of value %"PRIu64"\n",
          operations->insert_ops[insert_index]->operation->value(), value);
        operations->insert_ops[insert_index]->matching_op = NULL;
        insert_index++;
        if (insert_index >= operations->num_insert_ops) {
          fprintf(stderr,
              "FATAL13: The value %"PRId64" gets removed but never inserted.\n", value);
          exit(13);
        }
      }

      if (operations->insert_ops[insert_index]->operation->value() != value) {
        fprintf(stderr,
            "FATAL14: The value %"PRId64" gets removed but never inserted.\n", value);
        exit(14);

      }

      i++;

      operations->insert_ops[insert_index]->matching_op =
        operations->remove_ops[remove_index];
      operations->remove_ops[remove_index]->matching_op =
        operations->insert_ops[insert_index];

      insert_index++;
    }
    else {
      operations->remove_ops[remove_index]->matching_op =
        operations->remove_ops[remove_index];
      operations->remove_ops[remove_index]->insert_added = true;
    }
  }

  printf("matching operations: %d\n", i);
}

void create_insert_and_remove_array(Operations* operations, Operation** ops, int num_ops) {
  // Create two arrays which contain the insert operations and the remove
  // operations.
  operations->insert_ops = new Node*[num_ops];
  operations->remove_ops = new Node*[num_ops];

  int next_insert = 0;
  int next_remove = 0;
  for (int i = 0; i < num_ops; i++) {

    Node* node = new Node();
    node->operation = ops[i];
    node->costs = -1;
    node->insert_added = false;
    if (ops[i]->type() == Operation::INSERT) {
      operations->insert_ops[next_insert] = node;
      next_insert++;
    } else {
      operations->remove_ops[next_remove] = node;
      next_remove++;
    }
  }

  assert(next_insert + next_remove == num_ops);
  operations->num_insert_ops = next_insert;
  operations->num_remove_ops = next_remove;
  printf("Arrays created; %d insert operations and %d remove operations\n",
      next_insert, next_remove);

}

void create_doubly_linked_lists(Node* head, Node** ops, int num_ops) {

  qsort(ops, num_ops, sizeof(Node*), compare_operations_by_start_time);

  for (int i = 1; i < num_ops - 1; i++) {
    ops[i]->next = ops[i + 1];
    ops[i]->prev = ops[i - 1];
  }

  ops[0]->next = ops[1];
  ops[0]->prev = head;

  ops[num_ops - 1]->next = head;
  ops[num_ops - 1]->prev = ops[num_ops - 2];

  head->next = ops[0];
  head->prev = ops[num_ops - 1];

  printf("list created\n");
}

