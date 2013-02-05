#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "linearizer.h"
#include <limits.h>

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

void match_operations(Operations* operations);
void create_insert_and_remove_array(Operations* operations, Operation** ops, int num_ops);
void create_doubly_linked_lists(Node* head, Node** ops, int num_ops);
void initialize(Operations* operations, Operation** ops, int num_ops);
void linearize_remove_ops(Operations* operations);
void linearize_insert_ops(Operations* operations);
void next_order(Order** order, Operation* operation, int* next_index);
Order** merge_linearizations(Operations* operations);

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

int compare_operations_by_order(const void* left, const void* right) {
  return (*(Node**) left)->order > (*(Node**) right)->order;
}

Order** merge_linearizations(Operations* operations) {

  Order** result = new Order*[operations->num_insert_ops + operations->num_remove_ops];

  qsort(operations->insert_ops, operations->num_insert_ops, sizeof(Node*), compare_operations_by_order);
  qsort(operations->remove_ops, operations->num_remove_ops, sizeof(Node*), compare_operations_by_order);

  int next_index = 0;

  int j = 0;
  int i = 0;
  while (i < operations->num_insert_ops || j < operations->num_remove_ops) {

    if (i >= operations->num_insert_ops) {
      Node* remove_op = operations->remove_ops[j];
      next_order(result, remove_op->operation, &next_index);
      j++;
    } else if (j >= operations->num_remove_ops) {
      Node* insert_op = operations->insert_ops[i];
      next_order(result, insert_op->operation, &next_index);
      insert_op->matching_op->insert_added = true;
      i++;
    } else {
      Node* remove_op = operations->remove_ops[j];
      Node* insert_op = operations->insert_ops[i];
      if (remove_op->insert_added && 
          remove_op->operation->start() < insert_op->latest_lin_point) {
        next_order(result, remove_op->operation, &next_index);
        j++;
      } else {
        next_order(result, insert_op->operation, &next_index);
        insert_op->matching_op->insert_added = true;
        i++;
      }
    }
  }

  return result;
}

Order** linearize_by_min_max(Operation** ops, int num_ops) {

  Operations* operations = new Operations();

  initialize(operations, ops, num_ops);

  linearize_remove_ops(operations);
  linearize_insert_ops(operations);

  return merge_linearizations(operations);

}

int get_insert_costs(Operations* operations, Node* insert_op, 
    int maximal_costs, uint64_t first_end, uint64_t* selectable_bound) {

  assert (insert_op != NULL);
  assert (insert_op->matching_op != NULL);

  int costs = maximal_costs;
  Node* next_op = insert_op->next;
  while (next_op != operations->insert_list &&
      next_op->operation->start() <= insert_op->operation->end()) {

    if (next_op->matching_op->order < costs) {
      costs = next_op->matching_op->order;
      if (next_op->operation->start() > first_end) {
        *selectable_bound =next_op->operation->start();
        return costs;
      }
    }
    next_op = next_op->next;
  }

  return costs;
}

void next_order(Order** order, Operation* operation, int* next_index) {

  Order* new_order = new Order();
  new_order->operation = operation;
  new_order->order = (uint64_t)(*next_index);
  order[*next_index] = new_order;
  (*next_index)++;
}

void linearize_insert_ops(Operations* operations) {

  operations->insert_list = new Node();
  operations->insert_list->operation = NULL;
  create_doubly_linked_lists(operations->insert_list,
      operations->insert_ops, operations->num_insert_ops);

  int next_order = 0;

  while (operations->insert_list->next != operations->insert_list) {

    Node* op = operations->insert_list->next;

    int minimal_order = INT_MAX;

    uint64_t first_end = op->operation->end();
    Node* tmp = op;
    // Calculate the end of the first overlap group.
    while (tmp != operations->insert_list && tmp->operation->start() <= first_end) {

      if (tmp->operation->end() < first_end) {
        first_end = tmp->operation->end();
      }

      if (tmp->matching_op->order < minimal_order) {
        minimal_order = tmp->matching_op->order;
      }

      tmp = tmp->next;
    }

    // Selectable bound: operations which respond after the selectable bound
    // cannot be selected. The selectable bound is the invocation of the first
    // operation after the first overlap group whose matching remove operation
    // precedes all remove operation which match with an insert operation in the first
    // overlap group.
    uint64_t selectable_bound = (uint64_t)-1;

    // There is no need to calculate the costs of operations which respond
    // before the last_end because its costs cannot be lower than the costs of
    // already calculated operations.
//    uint64_t last_end = 0;

    Node* maximal_costs_op = op;
    int maximal_costs = -1;

    while (op != operations->insert_list && op->operation->start() <= first_end) {
    
      if (op->operation->end() < selectable_bound) {

  //      if (op->operation->end() > last_end) {
        
  //        last_end = op->operation->end();
          int costs = get_insert_costs(operations, op, minimal_order,
              first_end, &selectable_bound);

          if (costs > maximal_costs) {

            maximal_costs = costs;
            maximal_costs_op = op;
          } else if (costs == maximal_costs &&
              op->matching_op->order < maximal_costs_op->matching_op->order) {

            maximal_costs_op = op;
          }
//        }
      }
      op = op->next;
    }

    remove_from_list(maximal_costs_op);
    maximal_costs_op->order = next_order;
    next_order++;
    maximal_costs_op->latest_lin_point = first_end;

  }
  delete(operations->insert_list);
  operations->insert_list = NULL;
}

///
// This function calculates the costs of remove operations.
//
// operations All operations.
// remove_op The remove operation the costs are calculated for.
int get_remove_costs(Operations* operations, Node* remove_op) {

  Node* matching_insert = remove_op->matching_op;
  int costs = 0;
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
      costs++;
    }

    next_op = next_op->next;
  }
  assert(costs >= 0);
  return costs;
}

void linearize_remove_ops(Operations* operations) {

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

    // Only operations which start before the first remove operation ends are
    // within the first overlap group.
    while (op != operations->remove_list && op->operation->start() <= first_end) {

      int minimal_costs = -1;
      if (op->operation->end() < first_end) {
        first_end = op->operation->end();
      }

      if (op == minimal_costs_op) {
        // This op is already the minimal_costs_op.
      } else if (op->matching_op->operation->start() > 
          minimal_costs_op->matching_op->operation->end()) {
        // This op cannot have lower costs than the minimal_costs_op.
      } else if (op->matching_op->operation->end() <
          minimal_costs_op->matching_op->operation->start()) {
        minimal_costs_op = op;
        minimal_costs = -1;
        // This op has definitely lower costs than the minimal_costs_op.
      } else {

        if (minimal_costs == -1) {
          // We have not yet calculated the minimal costs.
          minimal_costs = get_remove_costs(operations, minimal_costs_op);
        }
        int costs = get_remove_costs(operations, op);
        if (costs < minimal_costs) {

          minimal_costs = costs;
          minimal_costs_op = op;
        } else if (costs == minimal_costs &&
            op->matching_op->operation->start() <
            minimal_costs_op->matching_op->operation->start()) {

          minimal_costs_op = op;
        }
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
  }

  delete(operations->remove_list);
  operations->remove_list = NULL;
  delete(operations->insert_list);
  operations->insert_list = NULL;
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
}

