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

int get_remove_costs(Operations* operations, Node* remove_op, Node* last_selected_op) {

  // If we have calculated the costs already once, we can reuse the result.
  if (remove_op->costs >= 0) {
    if (last_selected_op != NULL) {
      if (last_selected_op->matching_op->operation->end() < remove_op->matching_op->operation->start()) {
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
    int costs = 0;
    Node* next_op = operations->insert_list->next;
    while (next_op != operations->insert_list) {
      
      // No further insert operations can be found which strictly precede the
      // matching insert operation.
      if (next_op->operation->start() >= matching_insert->operation->start()) {
        remove_op->costs = costs;
        break;
      }
      // The operation op strictly precedes matching_insert. Costs increase by
      // one.
      if (next_op->operation->end() < matching_insert->operation->start()) {
        costs++;
      }

      next_op = next_op->next;
    }
  }
  assert(remove_op->costs >= 0);
  return remove_op->costs;
}

Order** linearize(Operation** ops, int num_ops) {
  
  Operations* operations = new Operations();

  initialize(operations, ops, num_ops); 
 
  int x = 0;
  int64_t total_costs = 0;
  Node* last_selected = NULL;

  while (operations->remove_list->next != operations->remove_list) {

    Node* op = operations->remove_list->next;
    uint64_t first_end = op->operation->end();

    Node* minimal_costs_op = NULL;

    int minimal_costs = INT_MAX;

    while (op != operations->remove_list && op->operation->start() <= first_end) {

//      printf("1\n");
      if (op->operation->end() < first_end) {
        first_end = op->operation->end();
      }

      int costs = get_remove_costs(operations, op, last_selected);
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
    if (minimal_costs > 0) {
      printf("not minimal costs: %d\n", minimal_costs);
    }

    printf("remove %"PRIu64" - %"PRIu64"\n",
        minimal_costs_op->matching_op->operation->start(),
        minimal_costs_op->matching_op->operation->end());
    remove_from_list(minimal_costs_op);
    if (minimal_costs_op != minimal_costs_op->matching_op) {
      remove_from_list(minimal_costs_op->matching_op);
    }
    last_selected = minimal_costs_op;
    x++;
    total_costs += minimal_costs;

  }

  printf("After list iteration x=%d with %"PRId64" costs\n", x, total_costs);
  return 0;
}

void initialize(Operations* operations, Operation** ops, int num_ops) {

  create_insert_and_remove_array(operations, ops, num_ops);
  match_operations(operations);

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list, 
      operations->insert_ops, operations->num_insert_ops);

  operations->remove_list = new Node();
  operations->remove_list->operation = NULL;
  create_doubly_linked_lists(operations->remove_list, 
      operations->remove_ops, operations->num_remove_ops);
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
    }
    else {
      operations->remove_ops[remove_index]->matching_op =
        operations->remove_ops[remove_index];
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

