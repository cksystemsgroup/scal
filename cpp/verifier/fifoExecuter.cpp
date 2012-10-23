#include "fifoExecuter.h"
#include "histogram.h"
#include <stdlib.h>
#include <list>
#include <algorithm>
#include <stdio.h>

#include <assert.h>

/**
 * Returns the number of enqueue operations which return before the invocation of the given remove
 * operation.
 */
int FifoExecuter::amountOfFinishedEnqueueOperations(
    Operation* removeOperation) const {
  int count = 0;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get();
      iter.valid() && element != removeOperation; element = iter.step()) {
    if (element->type() == Operation::INSERT
        && element->end() < removeOperation->start()) {
      count++;
    } else if (element == removeOperation) {
      break;
    }
  }
  return count;
}

int FifoExecuter::amountOfStartedEnqueueOperations(
    Operation* removeOperation) const {
  int count = 0;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get();
      iter.valid() && element->start() <= removeOperation->end();
      element = iter.step()) {
    if (element->type() == Operation::INSERT) {
      count++;
    }
  }
  return count;
}

void FifoExecuter::execute(Histogram* histogram) {
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {

    executeOperation(element, histogram);
  }
}

void FifoExecuter::executeOperation(Operation* element, Histogram* histogram) {
  if (element->type() != Operation::REMOVE) {
    return;
  }

  executeOperationWithOverlaps(element, histogram);
}

//int compare_operations_by_start_time(const void* left, const void* right) {
//  return (*(Operation**) left)->start() > (*(Operation**) right)->start();
//}
//
//int compare_ops_by_value(const void* left, const void* right) {
//  return (*(Operation**) left)->value() > (*(Operation**) right)->value();
//}

bool compare_by_order(Operation* left, Operation* right) {

  return left->order() < right->order();
}

void FifoExecuter::calculate_response_order() {

  qsort(ops_->all_ops(), ops_->num_all_ops(), sizeof(Operation*),
      Operation::compare_operations_by_end_time);

  for (int i = 0; i < ops_->num_all_ops(); i++) {
    Operation* op = ops_->all_ops()[i];

    op->set_lin_order(i);
  }
}

void FifoExecuter::calculate_order() {

  Operation insert_head(0, 0, 0);

  Operations::create_doubly_linked_list(&insert_head, ops_->insert_ops(),
      ops_->num_insert_ops());

  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*),
      Operation::compare_operations_by_order);

  Operations::Iterator iter = ops_->elements(&insert_head);

  int next_lin_order = 0;
  int remove_index = 0;

  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {

    std::sort(element->overlaps_ops_same_type_.begin(),
        element->overlaps_ops_same_type_.end(), compare_by_order);

    bool earlier_found;
    Operation* earlier_op;

    do {
      earlier_found = false;
      earlier_op = element;
      Time linearizable_time_window_end = element->end();
      for (size_t i = 0; i < element->overlaps_ops_same_type_.size(); i++) {

        Operation* tmp = element->overlaps_ops_same_type_[i];
        if (tmp->deleted()) {
          continue;
        }

        // The operation tmp is within the first overlap group, check if it should happen earlier.
        if (tmp->start() <= linearizable_time_window_end) {

          // The remove operation matching tmp occurs before the remove operation of element, so
          // tmp should happen before element.
          if (tmp->matching_op()->order()
              < earlier_op->matching_op()->order()) {
            earlier_op = tmp;
            earlier_found = true;
          }

          // Adjust the the end of the first overlap group.
          if (tmp->end() < linearizable_time_window_end) {
            linearizable_time_window_end = tmp->end();
          }
        }
      }

      // Remove first all remove operations which started before the earliest insert operation
      // still in the list.
      while (remove_index < ops_->num_remove_ops()
          && ops_->remove_ops()[remove_index]->start() < earlier_op->start()) {
        ops_->remove_ops()[remove_index]->set_lin_order(next_lin_order);
//        ops_->remove_ops()[remove_index]->print();
        next_lin_order++;
        remove_index++;

      }

      // Remove the insert operation.
      earlier_op->set_lin_order(next_lin_order);
      next_lin_order++;
      earlier_op->remove();
//      earlier_op->print();
    } while (earlier_found);
  }

  // Remove all remaining remove operations.
  for (; remove_index < ops_->num_remove_ops(); remove_index++) {
    ops_->remove_ops()[remove_index]->set_lin_order(next_lin_order);
    next_lin_order++;
//    ops_->remove_ops()[remove_index]->print();
  }
}

void FifoExecuter::mark_long_ops(Operation** upper_bounds, bool* ignore_flags) {
  Operation* current = NULL;
  int counter = 0;
  for (int i = ops_->num_insert_ops() - 1; i >= 0; i--) {
    if (current != upper_bounds[i]) {
      current = upper_bounds[i];
      counter = 0;
    }
    counter++;
    if (counter > 1000) {
      ignore_flags[current->lin_order()] = true;
    }
  }
}

void FifoExecuter::calculate_upper_bounds(bool* ignore_flags,
    Operation** upper_bounds) {
  Operation* lowest = ops_->insert_ops()[ops_->num_insert_ops() - 1];
  for (int i = ops_->num_insert_ops() - 1; i >= 0; i--) {

    Operation* op = ops_->insert_ops()[i];
    if (!ignore_flags[op->lin_order()]
        && op->lin_order() < lowest->lin_order()) {
      lowest = op;
    }
    upper_bounds[i] = lowest;
  }
}

inline bool is_prophetic_insert(Operation* op) {

  // Remove operations cannot be prophetic inserts.
  if (op->type() == Operation::REMOVE) {
    return false;
  }
  // Prophetic inserts only exist with a matching remove operation.
  if (op->matching_op() == NULL) {
    return false;
  }
  // If the matching remove operation is prophetic, then this insert operation is prophetic.
  return (op->real_start() > op->matching_op()->real_start());
}

inline bool is_prophetic_remove(Operation* op) {
  if (op->type() == Operation::INSERT) {
    return false;
  }

  // Null returns can never be prophetic.
  if (op->value() == -1) {
    return false;
  }

  assert(op->matching_op() != NULL);

  // A remove operation is prophetic if it occurs before the matching insert operation.
  return (op->real_start() < op->matching_op()->real_start());
}

/**
 * Calculates the element-fairness of all elements with prophetic dequeues.
 * The operations of these elements are flagged such that they get ignored
 * in the calculation of the element-fairness of all other elements.
 */
void ef_prophetic_dequeue(Operation** ops, int num_ops) {

  int num_prophetics = 0;
  int total_element_fairness = 0;
  int num_queue_elements = 0;

  for (int i = 0; i < num_ops; i++) {

    Operation* op = ops[i];

    if (op->type() == Operation::INSERT) {

      if (is_prophetic_insert(op)) {
        // Do nothing. Prophetic dequeues do not change the state.
      } else {
        num_queue_elements++;
      }
    } else {

      if (is_prophetic_remove(op)) {

        num_prophetics++;

        // Count all insert-operations and null-returns between the
        // prophetic dequeue and its matching enqueue.
        int counter = 0;
        for(int j = i + 1; j < num_ops && ops[j] != op->matching_op(); j++) {
          if (ops[j]->type() == Operation::INSERT) {
            // Check if it is a prophetic insert.
            if(is_prophetic_insert(ops[j])) {
              // Prophetic inserts increase the counter only if the matching remove
              // operation succeeds op.
              if(ops[j]->matching_op()->real_start() > op->real_start()) {
                counter++;
              }
            } else {
              counter++;
            }
          } else {
            // Only null-returns increase the counter.
            if(ops[j]->value() == -1) {
              counter ++;
            }
          }
        }
        op->set_overtakes(counter + num_queue_elements);

        total_element_fairness += counter + num_queue_elements;

      } else if(op->value() == -1) {
        // Null returns do not change the state. Do nothing.
      } else {
        num_queue_elements--;
      }
    }
  }

  double average = total_element_fairness;
    average /= num_prophetics;


  printf("prophetic dequeues: #; total; average: %d %d %.3f \n", num_prophetics, total_element_fairness, average);
}

void ef_null_returns(Operation** ops, int num_ops) {

  int num_queue_elements = 0;
  int total_element_fairness = 0;
  int num_null_returns = 0;

  for (int i = 0; i < num_ops; i++) {

    Operation* op = ops[i];

    if (op->type() == Operation::REMOVE) {
      if (op->value() == -1) {
        op->set_overtakes(num_queue_elements);
        total_element_fairness += num_queue_elements;
        num_null_returns++;
      } else if (!is_prophetic_remove(op)) {
        num_queue_elements--;
      }
    } else {
      if(!is_prophetic_insert(op)) {
        num_queue_elements++;
      }
    }

  }

  double average = total_element_fairness;
      average /= num_null_returns;


    printf("null returns: #; total; average: %d %d %.3f \n", num_null_returns, total_element_fairness, average);

}

void FifoExecuter::calculate_new_element_fairness() {

  qsort(ops_->all_ops(), ops_->num_all_ops(), sizeof(Operation*),
      Operation::compare_operations_by_real_start_time);

  // 1.) Calculate element-fairness for all elements with prophetic dequeues.
  ef_prophetic_dequeue(ops_->all_ops(), ops_->num_all_ops());
  // 2.) Calculate element-fairness for all null-returns
  ef_null_returns(ops_->all_ops(), ops_->num_all_ops());
  // 3.) Calculate element-fairness for all other elements
}

void FifoExecuter::calculate_element_fairness() {

  // 1.) Use the start times of the remove operations to determine
  //     the "actual linearization" of the insert operations.
  // 2.) Calculate upper bounds where we may find overlappings.
  // 3.) Calculate element fairness.

  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*),
      Operation::compare_operations_by_real_start_time);

  for (int i = 0; i < ops_->num_remove_ops(); i++) {

    Operation* op = ops_->remove_ops()[i];
    op->matching_op()->set_lin_order(i);
  }

  qsort(ops_->insert_ops(), ops_->num_insert_ops(), sizeof(Operation*),
      Operation::compare_operations_by_real_start_time);

  Operation** upper_bounds = (Operation**) malloc(
      ops_->num_insert_ops() * sizeof(Operation*));
  bool* ignore_flags = (bool*) malloc(ops_->num_remove_ops() * sizeof(bool));

  for (int i = 0; i < ops_->num_remove_ops(); i++) {
    ignore_flags[i] = false;
  }

  calculate_upper_bounds(ignore_flags, upper_bounds);

  for (int i = 0; i < 0; i++) {
    mark_long_ops(upper_bounds, ignore_flags);
    calculate_upper_bounds(ignore_flags, upper_bounds);
  }

  int ignore_counter = 0;

  for (int i = ops_->num_insert_ops() - 1; i >= 0; i--) {

    Operation* op = ops_->insert_ops()[i];

    if (ignore_flags[op->lin_order()]) {
      ignore_counter++;
    }
  }

  for (int i = 0; i < ops_->num_insert_ops(); i++) {

    Operation* op = ops_->insert_ops()[i];

    if (!ignore_flags[op->lin_order()]) {

      for (int j = i + 1;
          j < ops_->num_insert_ops()
              && op->lin_order() > upper_bounds[j]->lin_order(); j++) {

        Operation* inner_op = ops_->insert_ops()[j];

        if (inner_op->lin_order() < op->lin_order()) {
          op->increment_lateness();
          inner_op->increment_age();
        }
      }
    }
  }

  int fairness = 0;
  int num_aged = 0;
  int num_late = 0;
  int max_age = 0;
  int max_lateness = 0;
  // Count fairness, age, and lateness of all elements.
  for (int i = 0; i < ops_->num_insert_ops(); i++) {
    Operation* op = ops_->insert_ops()[i];

    if (op->age() > 0) {
      num_aged++;
      if (max_age < op->age()) {
        max_age = op->age();
      }
      fairness += op->age();
    }

    if (op->lateness() > 0) {
      num_late++;
      if (max_lateness < op->lateness()) {
        max_lateness = op->lateness();
      }
    }
  }

  double avg_fairness = fairness;
  avg_fairness /= ops_->num_remove_ops();

  printf("%d %.3f %d %d %d %d \n", fairness, avg_fairness, num_aged, max_age,
      num_late, max_lateness);
}

void FifoExecuter::calculate_op_fairness() {

  // Calculate the fairness of remove operations.
  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*),
      Operation::compare_operations_by_real_start_time);

  for (int i = 0; i < ops_->num_remove_ops(); i++) {

    Operation* op = ops_->remove_ops()[i];
    Time upper_time_bound = op->end();

    for (int j = i + 1; j < ops_->num_remove_ops(); j++) {

      Operation* inner_op = ops_->remove_ops()[j];
      if (inner_op->real_start() > upper_time_bound) {
        // All following operations are started after the current operation has ended, there will
        // be no more overtakings.
        break;
      }

      if (inner_op->lin_order() < op->lin_order()) {
        // The operation inner_op has overtaken the operation op.
        op->increment_lateness();
        inner_op->increment_age();
      } else {
        if (inner_op->end() < upper_time_bound) {
          // We adjust the upper_time_bound: if the operation inner_op responds earlier
          // but has taken effect later the operation op has taken effect before the end of
          // the operation inner_op.
          upper_time_bound = inner_op->end();
        }
      }
    }
  }

  int fairness_remove = 0;
  int num_aged_remove = 0;
  int num_late_remove = 0;
  int max_age_remove = 0;
  int max_lateness_remove = 0;
  // Count fairness, age, and lateness of all remove operations.
  for (int i = 0; i < ops_->num_remove_ops(); i++) {
    Operation* op = ops_->remove_ops()[i];

    if (op->age() > 0) {
      num_aged_remove++;
      if (max_age_remove < op->age()) {
        max_age_remove = op->age();
      }
      fairness_remove += op->age();
    }

    if (op->lateness() > 0) {
      num_late_remove++;
      if (max_lateness_remove < op->lateness()) {
        max_lateness_remove = op->lateness();
      }
    }
  }

  double avg_fairness_remove = fairness_remove;
  avg_fairness_remove /= ops_->num_remove_ops();

  // Calculate the fairness of insert operations.
  // We can use the precalculations of calculateOverlaps here because the start time of
  // insert operations is not changed in the initialization.
  for (int i = 0; i < ops_->num_insert_ops(); i++) {
    Operation* op = ops_->all_ops()[i];

    for (size_t j = 0; j < op->overlaps_ops_same_type_.size(); j++) {
      Operation* inner_op = op->overlaps_ops_same_type_[j];

      // op started before inner_op, if the lin_order is higher, then
      // the operation op is late.
      if (op->lin_order() > inner_op->lin_order()) {

        op->increment_lateness();
        inner_op->increment_age();
      }
    }
  }

  int fairness_insert = 0;
  int num_aged_insert = 0;
  int num_late_insert = 0;
  int max_age_insert = 0;
  int max_lateness_insert = 0;
  // Count fairness, age, and lateness of all insert operations.
  for (int i = 0; i < ops_->num_insert_ops(); i++) {
    Operation* op = ops_->insert_ops()[i];

    if (op->age() > 0) {
      num_aged_insert++;
      if (max_age_insert < op->age()) {
        max_age_insert = op->age();
      }
      fairness_insert += op->age();
    }

    if (op->lateness() > 0) {
      num_late_insert++;
      if (max_lateness_insert < op->lateness()) {
        max_lateness_insert = op->lateness();
      }
    }
  }

  double avg_fairness_insert = fairness_insert;
  avg_fairness_insert /= ops_->num_insert_ops();

  printf("%d %.3f %d %d %d %d %d %.3f %d %d %d %d\n", fairness_insert,
      avg_fairness_insert, num_aged_insert, max_age_insert, num_late_insert,
      max_lateness_insert,

      fairness_remove, avg_fairness_remove, num_aged_remove, max_age_remove,
      num_late_remove, max_lateness_remove);
}
