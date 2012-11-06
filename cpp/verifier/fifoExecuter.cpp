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

  Operations::create_doubly_linked_list(ops_->head(), ops_->all_ops(),
      ops_->num_all_ops(), Operation::compare_operations_by_start_time);
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
      ops_->num_insert_ops(), Operation::compare_operations_by_start_time);

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

inline bool is_prophetic_insert(Operation* op,
    uint64_t (*lin_point)(Operation* op)) {

  // Remove operations cannot be prophetic inserts.
  if (op->type() == Operation::REMOVE) {
    return false;
  }
  // Prophetic inserts only exist with a matching remove operation.
  if (op->matching_op() == NULL) {
    return false;
  }
  // If the matching remove operation is prophetic, then this insert operation is prophetic.
  return (lin_point(op) > lin_point(op->matching_op()));
}

inline bool is_prophetic_remove(Operation* op,
    uint64_t (*lin_point)(Operation* op)) {
  if (op->type() == Operation::INSERT) {
    return false;
  }

  // Null returns can never be prophetic.
  if (op->value() == -1) {
    return false;
  }

  assert(op->matching_op() != NULL);

  // A remove operation is prophetic if it occurs before the matching insert operation.
  return (lin_point(op) < lin_point(op->matching_op()));
}

/**
 * Calculates the element-fairness of all elements with prophetic dequeues.
 * The operations of these elements are flagged such that they get ignored
 * in the calculation of the element-fairness of all other elements.
 */
void ef_prophetic_dequeue(Operation** ops, int64_t num_ops,
    int64_t* num_prophetics, int64_t* total, int64_t* max,
    uint64_t (*lin_point)(Operation* op)) {

  *num_prophetics = 0;
  *total = 0;
  *max = 0;
  int num_queue_elements = 0;

  for (int i = 0; i < num_ops; i++) {

    Operation* op = ops[i];

    if (op->type() == Operation::INSERT) {

      if (is_prophetic_insert(op, lin_point)) {
        // Do nothing. Prophetic dequeues do not change the state.
      } else {
        num_queue_elements++;
      }
    } else {

      if (is_prophetic_remove(op, lin_point)) {

        (*num_prophetics)++;

        // Count all insert-operations and null-returns between the
        // prophetic dequeue and its matching enqueue.
        int counter = 0;
        for (int j = i + 1; j < num_ops && ops[j] != op->matching_op(); j++) {
          if (ops[j]->type() == Operation::INSERT) {
            // Check if it is a prophetic insert.
            if (is_prophetic_insert(ops[j], lin_point)) {
              // Prophetic inserts increase the counter only if the matching remove
              // operation succeeds op.
              if (lin_point(ops[j]->matching_op()) > lin_point(op)) {
                counter++;
              }
            } else {
              counter++;
            }
          } else {
            // Only null-returns increase the counter.
            if (ops[j]->value() == -1) {
              counter++;
            }
          }
        }
        op->set_overtakes(counter + num_queue_elements);

        int out_of_order = counter + num_queue_elements;
        if (*max < out_of_order) {
          *max = out_of_order;
        }
        *total += out_of_order;

      } else if (op->value() == -1) {
        // Null returns do not change the state. Do nothing.
      } else {
        num_queue_elements--;
      }
    }
  }
}

void ef_null_returns(Operation** ops, int num_ops, int64_t* num_null_returns,
    int64_t* total, int64_t* max, uint64_t (*lin_point)(Operation* op)) {

  *num_null_returns = 0;
  *total = 0;
  *max = 0;

  int num_queue_elements = 0;

  for (int i = 0; i < num_ops; i++) {

    Operation* op = ops[i];

    if (op->type() == Operation::REMOVE) {
      if (op->value() == -1) {
        op->set_overtakes(num_queue_elements);

        if (*max < num_queue_elements) {
          *max = num_queue_elements;
        }
        *total += num_queue_elements;
        (*num_null_returns)++;
      } else if (!is_prophetic_remove(op, lin_point)) {
        num_queue_elements--;
      }
    } else {
      if (!is_prophetic_insert(op, lin_point)) {
        num_queue_elements++;
      }
    }

  }
}

void ef_normal_dequeues(Operation** insert_ops, int num_insert_ops,
    Operation** remove_ops, int num_remove_ops, int64_t* num_normal,
    int64_t* total, int64_t* max, uint64_t (*lin_point)(Operation* op),
    int (*compare_ops)(const void* left, const void* right)) {
  // 1.) Calculate upper bounds up to which it is possible to find other elements whose
  //     which get overtaken by the current operation.
  // 2.) Starting from a remove operation, count all other remove operations whose matching insert
  //     operations were invoked earlier than the insert operation of the remove operation.

  *num_normal = 0;
  *total = 0;
  *max = 0;

  Operation head;
  Operations::create_doubly_linked_list(&head, insert_ops, num_insert_ops,
      compare_ops);

  qsort(remove_ops, num_remove_ops, sizeof(Operation*), compare_ops);

  for (int i = 0; i < num_remove_ops; i++) {

    Operation* op = remove_ops[i];

    if (op->value() < 0) {
      // Null return: do nothing.
    } else if (is_prophetic_remove(op, lin_point)) {
      // Prophetic dequeue, do nothing.
    } else {

      Operations::Iterator iter(&head);
      int counter = 0;
      for (Operation* element = iter.get();
          iter.valid() && element != op->matching_op(); element = iter.step()) {

        if (is_prophetic_insert(element, lin_point)) {
          element->remove();
          // Do nothing, the element got dequeued already.
        } else {
          counter++;
        }
      }

      op->matching_op()->remove();

      if (*max < counter) {
        *max = counter;
      }
      *total += counter;
      op->set_overtakes(counter);
      (*num_normal)++;
    }
  }

//  qsort(remove_ops, num_remove_ops, sizeof(Operation*), compare_ops);
//
//  // Step 1
//  Time* upper_bounds = (Time*) malloc(num_remove_ops * sizeof(Time));
//
//  Time earliestInsert = lin_point(
//      remove_ops[num_remove_ops - 1]->matching_op());
//
//  for (int i = num_remove_ops - 1; i >= 0; i--) {
//
//    if (remove_ops[i]->value() == -1) {
//      // Do nothing. Null returns already got handled.
//    } else if (is_prophetic_remove(remove_ops[i], lin_point)) {
//      // Do nothing. Prophetic dequeues already got handled.
//    } else {
//      if (lin_point(remove_ops[i]->matching_op()) < earliestInsert) {
//        earliestInsert = lin_point(remove_ops[i]->matching_op());
//      }
//    }
//    upper_bounds[i] = earliestInsert;
//  }
//
//  for (int i = 0; i < num_remove_ops; i++) {
//
//    if (i % 10000 == 0) {
//      printf("Progress: %d\n", i);
//    }
//
//    Operation* op = remove_ops[i];
//
//    if (op->value() == -1) {
//      // Do nothing for null returns, they have been handled already.
//    } else if (is_prophetic_remove(op, lin_point)) {
//      // Do nothing for prophetic removes, they have been handled already.
//    } else {
//      int counter = 0;
//      for (int j = i + 1;
//          j < num_remove_ops && lin_point(op->matching_op()) > upper_bounds[j];
//          j++) {
//
//        if (remove_ops[j]->value() == -1) {
//          // Do nothing for null returns, they have been handled already.
//        } else if (is_prophetic_remove(remove_ops[j], lin_point)) {
//          // Do nothing for prophetic removes, they have been handled already.
//        } else {
//          if (lin_point(remove_ops[j]->matching_op())
//              < lin_point(op->matching_op())) {
//            counter++;
//          }
//        }
//      }
//
//      if (*max < counter) {
//        *max = counter;
//      }
//      *total += counter;
//      op->set_overtakes(counter);
//      (*num_normal)++;
//    }
//  }
}

void ef_all_dequeues(Operations* ops, int64_t* num, int64_t* total,
    int64_t* max,

    int64_t* num_prophetic, int64_t* total_prophetic, int64_t* max_prophetic,

    int64_t* num_null, int64_t* total_null, int64_t* max_null,

    int64_t* num_normal, int64_t* total_normal, int64_t* max_normal,

    uint64_t (*lin_point)(Operation* op),
    int (*compare_ops)(const void* left, const void* right)) {

  qsort(ops->all_ops(), ops->num_all_ops(), sizeof(Operation*), compare_ops);

  *num_prophetic = 0;
  *total_prophetic = 0;
  *max_prophetic = 0;

  *num_null = 0;
  *total_null = 0;
  *max_null = 0;

  *num_normal = 0;
  *total_normal = 0;
  *max_normal = 0;

  // 1.) Calculate element-fairness for all elements with prophetic dequeues.
  ef_prophetic_dequeue(ops->all_ops(), ops->num_all_ops(), num_prophetic,
      total_prophetic, max_prophetic, lin_point);
  // 2.) Calculate element-fairness for all null-returns
  ef_null_returns(ops->all_ops(), ops->num_all_ops(), num_null, total_null,
      max_null, lin_point);
  // 3.) Calculate element-fairness for all other elements
  ef_normal_dequeues(ops->insert_ops(), ops->num_insert_ops(),
      ops->remove_ops(), ops->num_remove_ops(), num_normal, total_normal,
      max_normal, lin_point, compare_ops);

  *num = *num_prophetic + *num_null + *num_normal;
  *max =
      (*max_prophetic > *max_null) ?
          ((*max_prophetic > *max_normal) ? *max_prophetic : *max_normal) :
          ((*max_null > *max_normal) ? *max_null : *max_normal);
  *total = * total_prophetic + * total_null + *total_normal;
}

int64_t abs_diff(int64_t v1, int64_t v2) {
  if (v1 > v2) {
    return v1 - v2;
  }
  return v2 - v1;
}

void FifoExecuter::calculate_diff(
    uint64_t (*lin_point_1)(Operation* op),
    int (*compare_ops_1)(const void* left, const void* right),
    uint64_t (*lin_point_2)(Operation* op),
    int (*compare_ops_2)(const void* left, const void* right)) {

    int64_t num_prophetic1 = 0;
    int64_t total_prophetic1 = 0;
    int64_t max_prophetic1 = 0;

    int64_t num_null1 = 0;
    int64_t total_null1 = 0;
    int64_t max_null1 = 0;

    int64_t num_normal1 = 0;
    int64_t total_normal1 = 0;
    int64_t max_normal1 = 0;

    int64_t num1 = 0;
    int64_t max1 = 0;
    int64_t total1 = 0;

    ef_all_dequeues(ops_, &num1, &total1, &max1, &num_prophetic1, &total_prophetic1,
        &max_prophetic1, &num_null1, &total_null1, &max_null1, &num_normal1, &total_normal1,
        &max_normal1, lin_point_1, compare_ops_1);

    int64_t num_prophetic2 = 0;
    int64_t total_prophetic2 = 0;
    int64_t max_prophetic2 = 0;

    int64_t num_null2 = 0;
    int64_t total_null2 = 0;
    int64_t max_null2 = 0;

    int64_t num_normal2 = 0;
    int64_t total_normal2 = 0;
    int64_t max_normal2 = 0;

    int64_t num2 = 0;
    int64_t max2 = 0;
    int64_t total2 = 0;

    ef_all_dequeues(ops_, &num2, &total2, &max2, &num_prophetic2, &total_prophetic2,
        &max_prophetic2, &num_null2, &total_null2, &max_null2, &num_normal2, &total_normal2,
        &max_normal2, lin_point_2, compare_ops_2);

    int64_t num_prophetic = num_prophetic1;
    int64_t total_prophetic = abs_diff(total_prophetic1, total_prophetic2);

    int64_t num_null = num_null1;
    int64_t total_null = abs_diff(total_null1, total_null2);

    int64_t num_normal = num_normal1;
    int64_t total_normal = abs_diff(total_normal1, total_normal2);

    int64_t num = num1;
    int64_t total = abs_diff(total1, total2);

    double average_prophetic = total_prophetic;
    if (num_prophetic != 0) {
      average_prophetic /= num_prophetic;
    }

    double average_null = total_null;
    if (num_null != 0) {
      average_null /= num_null;
    }

    double average_normal = total_normal;
    if (num_normal != 0) {
      average_normal /= num_normal;
    }

    double average = total;
    if (num != 0) {
      average /= num;
    }

    printf(
        "%lu %.3f %lu %.3f %lu %.3f %lu %.3f\n",
        total, average, total_normal,
        average_normal, total_null, average_null,
        total_prophetic, average_prophetic);
}

void FifoExecuter::calculate_new_element_fairness(
    uint64_t (*lin_point)(Operation* op),
    int (*compare_ops)(const void* left, const void* right)) {

  int64_t num_prophetic = 0;
  int64_t total_prophetic = 0;
  int64_t max_prophetic = 0;

  int64_t num_null = 0;
  int64_t total_null = 0;
  int64_t max_null = 0;

  int64_t num_normal = 0;
  int64_t total_normal = 0;
  int64_t max_normal = 0;

  int64_t num = 0;
  int64_t max = 0;
  int64_t total = 0;

  ef_all_dequeues(ops_, &num, &total, &max, &num_prophetic, &total_prophetic,
      &max_prophetic, &num_null, &total_null, &max_null, &num_normal, &total_normal,
      &max_normal, lin_point, compare_ops);

  double average_prophetic = total_prophetic;
  if (num_prophetic != 0) {
    average_prophetic /= num_prophetic;
  }

  double average_null = total_null;
  if (num_null != 0) {
    average_null /= num_null;
  }

  double average_normal = total_normal;
  if (num_normal != 0) {
    average_normal /= num_normal;
  }

  double average = total;
  if (num != 0) {
    average /= num;
  }

  printf(
      "%lu %lu %lu %.3f %lu %lu %lu %.3f %lu %lu %lu %.3f %lu %lu %lu %.3f\n",
      num, max, total, average, num_normal, max_normal, total_normal,
      average_normal, num_null, max_null, total_null, average_null,
      num_prophetic, max_prophetic, total_prophetic, average_prophetic);

}

void FifoExecuter::calculate_element_fairness() {

// 1.) Use the start times of the remove operations to determine
//     the "actual linearization" of the insert operations.
// 2.) Calculate upper bounds where we may find overlappings.
// 3.) Calculate element fairness.

  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*),
      Operation::compare_operations_by_start_time);

  for (int i = 0; i < ops_->num_remove_ops(); i++) {

    Operation* op = ops_->remove_ops()[i];
    op->matching_op()->set_lin_order(i);
  }

  qsort(ops_->insert_ops(), ops_->num_insert_ops(), sizeof(Operation*),
      Operation::compare_operations_by_start_time);

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

void FifoExecuter::aggregate_semantical_error() {

  int64_t total = 0;
  int64_t max = 0;
  for (int i = 0; i < ops_->num_remove_ops(); i++) {
    Operation* op = ops_->remove_ops()[i];

    total += op->error();

    if (op->error() > max) {
      max = op->error();
    }
  }

  double avg = total;
  avg /= ops_->num_remove_ops();

  printf("%lu %.3f %lu\n", total, avg, max);
}

void FifoExecuter::calculate_op_fairness_typeless() {

  // Calculate the fairness of remove operations.
  qsort(ops_->all_ops(), ops_->num_all_ops(), sizeof(Operation*),
      Operation::compare_operations_by_start_time);

  for (int64_t i = 0; i < ops_->num_all_ops(); i++) {

    Operation* op = ops_->all_ops()[i];
    Time upper_time_bound = op->end();

    for (int64_t j = i + 1; j < ops_->num_all_ops(); j++) {

      Operation* inner_op = ops_->all_ops()[j];
      if (inner_op->start() > upper_time_bound) {
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

  int64_t fairness = 0;
  int64_t num_aged = 0;
  int64_t num_late = 0;
  int64_t max_age = 0;
  int64_t max_lateness = 0;
  // Count fairness, age, and lateness of all remove operations.
  for (int64_t i = 0; i < ops_->num_all_ops(); i++) {
    Operation* op = ops_->all_ops()[i];

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
  avg_fairness /= ops_->num_all_ops();

  printf("%lu %.3f %lu %lu %lu %lu\n", fairness, avg_fairness, num_aged,
      max_age, num_late, max_lateness);
}

void FifoExecuter::calculate_op_fairness() {

  // Calculate the fairness of remove operations.
  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*),
      Operation::compare_operations_by_start_time);

  for (int i = 0; i < ops_->num_remove_ops(); i++) {

    Operation* op = ops_->remove_ops()[i];
    Time upper_time_bound = op->end();

    for (int j = i + 1; j < ops_->num_remove_ops(); j++) {

      Operation* inner_op = ops_->remove_ops()[j];
      if (inner_op->start() > upper_time_bound) {
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

void FifoExecuter::calculate_performance_index() {
  qsort(ops_->all_ops(), ops_->num_all_ops(), sizeof(Operation*),
      Operation::compare_operations_by_start_time);

  // Allocate an array which stores the number of overlaps of each operation.
  int64_t* overlaps = (int64_t*) calloc(ops_->num_all_ops(), sizeof(int64_t));

  for (int i = 0; i < ops_->num_all_ops(); i++) {
    Operation* op = ops_->all_ops()[i];

    for (int j = i + 1;
        j < ops_->num_all_ops() && ops_->all_ops()[j]->start() <= op->end();
        j++) {

      overlaps[i]++;
      //     overlaps[j]++;
    }
  }

  int64_t max = 0;
  int64_t total = 0;

  for (int i = 0; i < ops_->num_all_ops(); i++) {
    if (overlaps[i] > max) {
      max = overlaps[i];
    }
    total += overlaps[i];
  }

  double avg = total;
  avg /= ops_->num_all_ops();

  printf("%lu %lu %.3f\n", max, total, avg);
  free(overlaps);
}
