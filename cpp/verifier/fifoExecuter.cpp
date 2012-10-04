
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
int FifoExecuter::amountOfFinishedEnqueueOperations(Operation* removeOperation) const {
  int count = 0;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get(); iter.valid() && element != removeOperation; element = iter.step()) {
    if (element->type() == Operation::INSERT &&
        element->end() < removeOperation->start()) {
      count++;
    } else if (element == removeOperation) {
     break;
    }
  }
  return count;
}

int FifoExecuter::amountOfStartedEnqueueOperations(Operation* removeOperation) const {
  int count = 0;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get(); iter.valid() && element->start() <= removeOperation->end(); element = iter.step()) {
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
  
  qsort(ops_->all_ops(), ops_->num_all_ops(), sizeof(Operation*), Operation::compare_operations_by_end_time);

  for (int i = 0; i < ops_->num_all_ops(); i++) {
    Operation* op = ops_->all_ops()[i];

    op->set_lin_order(i);
  }
}

void FifoExecuter::calculate_order() {

  Operation insert_head(0, 0, 0);

  Operations::create_doubly_linked_list(&insert_head, ops_->insert_ops(), ops_->num_insert_ops());

  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*), Operation::compare_operations_by_order);

  Operations::Iterator iter = ops_->elements(&insert_head);

  int next_lin_order = 0;
  int remove_index = 0;

  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {


    std::sort(element->overlaps_ops_same_type_.begin(), element->overlaps_ops_same_type_.end(), compare_by_order);

    bool earlier_found;
    Operation* earlier_op;

    do {
      earlier_found = false;
      earlier_op = element;
      Time linearizable_time_window_end = element->end();
      for(size_t i = 0; i < element->overlaps_ops_same_type_.size(); i++) {

        Operation* tmp = element->overlaps_ops_same_type_[i];
        if(tmp->deleted()) {
          continue;
        }

        // The operation tmp is within the first overlap group, check if it should happen earlier.
        if(tmp->start() <= linearizable_time_window_end) {

          // The remove operation matching tmp occurs before the remove operation of element, so
          // tmp should happen before element.
          if(tmp->matching_op()->order() < earlier_op->matching_op()->order()) {
            earlier_op = tmp;
            earlier_found = true;
          }

          // Adjust the the end of the first overlap group.
          if(tmp->end() < linearizable_time_window_end) {
            linearizable_time_window_end = tmp->end();
          }
        }
      }

      // Remove first all remove operations which started before the earliest insert operation
      // still in the list.
      while(remove_index < ops_->num_remove_ops() && ops_->remove_ops()[remove_index]->start() < earlier_op->start()) {
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
    }
    while(earlier_found);
  }

  // Remove all remaining remove operations.
  for (; remove_index < ops_->num_remove_ops(); remove_index++) {
    ops_->remove_ops()[remove_index]->set_lin_order(next_lin_order);
    next_lin_order++;
//    ops_->remove_ops()[remove_index]->print();
  }
}

int FifoExecuter::calculate_op_fairness() {

  // Calculate the fairness of remove operations.
  qsort(ops_->remove_ops(), ops_->num_remove_ops(), sizeof(Operation*), Operation::compare_operations_by_real_start_time);

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
      }
      else {
        if(inner_op->end() < upper_time_bound) {
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

    if(op->age() > 0) {
      num_aged_remove++;
      if(max_age_remove < op->age()) {
        max_age_remove = op->age();
      }
      fairness_remove += op->age();
    }

    if(op->lateness() > 0) {
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

    for(size_t j = 0; j < op->overlaps_ops_same_type_.size(); j++) {
      Operation* inner_op = op->overlaps_ops_same_type_[j];

      // op started before inner_op, if the lin_order is higher, then
      // the operation op is late.
      if(op->lin_order() > inner_op->lin_order()) {

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

    if(op->age() > 0) {
      num_aged_insert++;
      if(max_age_insert < op->age()) {
        max_age_insert = op->age();
      }
      fairness_insert += op->age();
    }

    if(op->lateness() > 0) {
      num_late_insert++;
      if (max_lateness_insert < op->lateness()) {
        max_lateness_insert = op->lateness();
      }
    }
  }

  double avg_fairness_insert = fairness_insert;
  avg_fairness_insert /= ops_->num_insert_ops();

  printf("%d %.3f %d %d %d %d %d %.3f %d %d %d %d\n",
      fairness_insert,
      avg_fairness_insert,
      num_aged_insert,
      max_age_insert,
      num_late_insert,
      max_lateness_insert,

      fairness_remove,
      avg_fairness_remove,
      num_aged_remove,
      max_age_remove,
      num_late_remove,
      max_lateness_remove
      );

  return 0;
//  int op_fairness = 0;
//  int num_late_ops = 0;
//  int num_aged_ops = 0;
//  int max_lateness = 0;
//  int max_age = 0;
//
//  // Calculate age and lateness of all operations.
//  for (int i = 0; i < ops_->num_all_ops(); i++) {
//    Operation* op = ops_->all_ops()[i];
//
//    for(size_t j = 0; j < op->overlaps_ops_same_type_.size(); j++) {
//      Operation* inner_op = op->overlaps_ops_same_type_[j];
//
//      // op started before inner_op, if the lin_order is higher, then
//      // the operation op is late.
//      if(op->lin_order() > inner_op->lin_order()) {
//
//        // This is the first time we see that op is late, so we increment
//        // the late-ops counter.
//        if (op->lateness() == 0) {
//          num_late_ops++;
//        }
//
//        op->increment_lateness();
//        inner_op->increment_age();
//        op_fairness++;
//      }
//    }
//
//    if (op->lateness() > max_lateness) {
//      max_lateness = op->lateness();
//    }
//  }
//
//  // Calculate the number of aged operations and the maximum age.
//  for (int i = 0; i < ops_->num_all_ops(); i++) {
//    Operation* op = ops_->all_ops()[i];
//
//    if (op->age() > 0) {
//      num_aged_ops++;
//    }
//
//    if (op->age() > max_age) {
//      max_age = op->age();
//    }
//  }
//
//  double avg_op_fairness = op_fairness;
//  avg_op_fairness /= ops_->num_all_ops();
//
//  printf("Total op-fairness: %d\n", op_fairness);
//  printf("Average op-fairness: %.3f\n", avg_op_fairness);
//  printf("Number of late ops: %d\n", num_late_ops);
//  return op_fairness;
}
