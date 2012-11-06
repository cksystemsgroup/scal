#include "fifoExecuterLowerBound.h"
#include "histogram.h"

#include <assert.h>

/**
 * Returns the number of insert operations which are strictly before the insert operation
 * corresponding to the given remove operation, i.e. the end time of the insert operation is
 * before the start time of the corresponding insert operation.
 */
int FifoExecuterLowerBound::semanticalError(const Operation* removeOperation,
    Operation** matchingInsertOperation) const {

  int semantical_error = 0;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {
    if (element->start() > removeOperation->end())
      break;

    if (element->type() == Operation::INSERT) {
      if (element->value() == removeOperation->value()) {
        if (matchingInsertOperation != NULL) {
          *matchingInsertOperation = element;
        } else {
          assert(false);
        }
        return semantical_error;
      }
      // This comparison does not check if the remove operation overlaps with the current
      // operation but if the insert operation corresponding to the remove operation overlaps
      // with the current operation.
      else if (element->end() < removeOperation->matching_op()->start()) {
        semantical_error++;
      }
    }
  }
  return -1;
}

void FifoExecuterLowerBound::executeOperationWithOverlaps(Operation* element,
    Histogram* histogram) {
  int error;
  Operation* matchingInsertOperation;
  bool null_return = (element->value() == -1);

  if (null_return) {
    // AH: The error of a null return is not the number of finished insert
    // operations before the null return but the number of finished insert
    // operations without matching remove operation before the current remove
    // operation.
    error = amountOfFinishedEnqueueOperations(element);
  } else {
    error = semanticalError(element, &matchingInsertOperation);
  }

  while (error > 0) {
    vector<ErrorEntry> overlaps;
    Time linearizable_time_window_end;
    element->determineExecutionOrderLowerBound(*ops_, this, error, &overlaps,
        &linearizable_time_window_end);
    for (size_t i = 0; i < overlaps.size(); i++) {
      assert(overlaps[i].error_ < error);
    }

    if (overlaps.size() == 0) {
      break;
    }
    bool done = false;
    // FIXME: debug check that overlaps are sorted correctly!
    for (size_t i = 0; i < overlaps.size(); i++) {

//      if ((i > 0) && (overlaps[i].error_ > 0)) {
//        // AH: The operation stored in overlaps[0] may be the one which returns first, hence without
//        // that operation the overlaps set may be bigger in the next iteration of the while loop.
//        // TODO: We can optimize here. We do not need to break as long as we have not processed the
//        // operation which returns first.
//        break;
//      }

      if (done) {
        break;
      }

      done = (overlaps[i].op_->end() == linearizable_time_window_end);

      int newError = semanticalError(overlaps[i].op_, &matchingInsertOperation);
      histogram->add(newError);

      ops_->remove(matchingInsertOperation, 0);
      ops_->remove(overlaps[i].op_, newError);
    }
    if (null_return) {
      error = amountOfFinishedEnqueueOperations(element);
    } else {
      error = semanticalError(element, &matchingInsertOperation);
    }
    if (error == -1) {
      fprintf(stderr, "FATAL1: errorDistance -1, element->value %d\n",
          element->value());
      exit(-1);
    }
  }

  if (error == -1) {
    fprintf(stderr, "FATAL2: errorDistance -1, element->value %d\n",
        element->value());
    exit(-1);
  }
  histogram->add(error);
  if (!null_return) {
    ops_->remove(matchingInsertOperation, 0);
  }
  ops_->remove(element, error);
  return;
}

//void FifoExecuterLowerBound::overlapTest1() {
//  Operations ops;
//
//  Operation* input[8];
//
//  for (uint64_t i = 0; i < 4; ++i) {
//    input[i] = new Operation(i * 2, i * 2 + 1, i * 2 + 1, Operation::INSERT, i + 1);
//  }
//
//  Operation r_0(10, 20, 20, Operation::REMOVE, 4);
//  input[4] = &r_0;
//  Operation r_1(12, 13, 13, Operation::REMOVE, 3);
//  input[5] = &r_1;
//  Operation r_2(14, 15, 15, Operation::REMOVE, 2);
//  input[6] = &r_2;
//  Operation r_3(16, 17, 17, Operation::REMOVE, 1);
//  input[7] = &r_3;
//
//  ops.Initialize(input, 8, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(input[4]->lateness() == 3);
//  assert(input[5]->lateness() == 0);
//  assert(input[6]->lateness() == 0);
//  assert(input[7]->lateness() == 0);
//  assert(input[4]->age() == 0);
//  assert(input[5]->age() == 1);
//  assert(input[6]->age() == 1);
//  assert(input[7]->age() == 1);
//
//  assert(input[0]->error() == 0);
//  assert(input[0]->order() == 4);
//  assert(input[1]->error() == 0);
//  assert(input[1]->order() == 2);
//  assert(input[2]->error() == 0);
//  assert(input[2]->order() == 0);
//  assert(input[3]->error() == 0);
//  assert(input[3]->order() == 6);
//  assert(input[4]->error() == 0);
//  assert(input[4]->order() == 7);
//  assert(input[5]->error() == 2);
//  assert(input[5]->order() == 1);
//  assert(input[6]->error() == 1);
//  assert(input[6]->order() == 3);
//  assert(input[7]->error() == 0);
//  assert(input[7]->order() == 5);
//}
//
//void FifoExecuterLowerBound::overlapTest2() {
//  Operations ops;
//
//  Operation* input[8];
//
//  Operation i_0(0, 1, 1, Operation::INSERT, 1);
//  Operation i_1(2, 3, 3, Operation::INSERT, 2);
//  Operation i_2(4, 5, 5, Operation::INSERT, 3);
//  Operation i_3(6, 7, 7, Operation::INSERT, 4);
//
//  input[0] = &i_0;
//  input[1] = &i_1;
//  input[2] = &i_2;
//  input[3] = &i_3;
//
//  Operation r_0(10, 13, 13, Operation::REMOVE, 4);
//  input[4] = &r_0;
//  Operation r_1(12, 15, 15, Operation::REMOVE, 3);
//  input[5] = &r_1;
//  Operation r_2(14, 17, 17, Operation::REMOVE, 2);
//  input[6] = &r_2;
//  Operation r_3(16, 19, 19, Operation::REMOVE, 1);
//  input[7] = &r_3;
//
//  ops.Initialize(input, 8, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(input[4]->lateness() == 1);
//  assert(input[5]->lateness() == 0);
//  assert(input[6]->lateness() == 1);
//  assert(input[7]->lateness() == 0);
//  assert(input[4]->age() == 0);
//  assert(input[5]->age() == 1);
//  assert(input[6]->age() == 0);
//  assert(input[7]->age() == 1);
//
//  assert(i_0.lin_order() == 0);
//  assert(i_1.lin_order() == 1);
//  assert(i_2.lin_order() == 2);
//  assert(i_3.lin_order() == 3);
//
//  assert(r_0.lin_order() == 5);
//  assert(r_1.lin_order() == 4);
//  assert(r_2.lin_order() == 7);
//  assert(r_3.lin_order() == 6);
////
////  assert(input[0]->error() == 0);
////  assert(input[0]->order() == 4);
////  assert(input[1]->error() == 0);
////  assert(input[1]->order() == 6);
////  assert(input[2]->error() == 0);
////  assert(input[2]->order() == 0);
////  assert(input[3]->error() == 0);
////  assert(input[3]->order() == 2);
////  assert(input[4]->error() == 2);
////  assert(input[4]->order() == 3);
////  assert(input[5]->error() == 2);
////  assert(input[5]->order() == 1);
////  assert(input[6]->error() == 0);
////  assert(input[6]->order() == 7);
////  assert(input[7]->error() == 0);
////  assert(input[7]->order() == 5);
//}
//
//void FifoExecuterLowerBound::overlapTest3() {
//  Operations ops;
//
//  Operation* input[8];
//
//  Operation i_0(0, 1, 1, Operation::INSERT, 1);
//  Operation i_1(2, 3, 3, Operation::INSERT, 2);
//  Operation i_2(4, 5, 5, Operation::INSERT, 3);
//  Operation i_3(6, 7, 7, Operation::INSERT, 4);
//
//  input[0] = &i_0;
//  input[1] = &i_1;
//  input[2] = &i_2;
//  input[3] = &i_3;
//
//  Operation r_0(10, 20, 20, Operation::REMOVE, 4);
//  input[4] = &r_0;
//  Operation r_1(12, 14, 14, Operation::REMOVE, 2);
//  input[5] = &r_1;
//  Operation r_2(13, 16, 16, Operation::REMOVE, 3);
//  input[6] = &r_2;
//  Operation r_3(15, 17, 17, Operation::REMOVE, 1);
//  input[7] = &r_3;
//
//  ops.Initialize(input, 8, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(input[4]->lateness() == 3);
//  assert(input[5]->lateness() == 0);
//  assert(input[6]->lateness() == 1);
//  assert(input[7]->lateness() == 0);
//  assert(input[4]->age() == 0);
//  assert(input[5]->age() == 1);
//  assert(input[6]->age() == 1);
//  assert(input[7]->age() == 2);
//
//  assert(i_0.lin_order() == 0);
//  assert(i_1.lin_order() == 1);
//  assert(i_2.lin_order() == 2);
//  assert(i_3.lin_order() == 3);
//
//  assert(r_0.lin_order() == 7);
//  assert(r_1.lin_order() == 4);
//  assert(r_2.lin_order() == 6);
//  assert(r_3.lin_order() == 5);
//
////  assert(input[0]->error() == 0);
////  assert(input[0]->order() == 2);
////  assert(input[1]->error() == 0);
////  assert(input[1]->order() == 0);
////  assert(input[2]->error() == 0);
////  assert(input[2]->order() == 4);
////  assert(input[3]->error() == 0);
////  assert(input[3]->order() == 6);
////  assert(input[4]->error() == 0);
////  assert(input[4]->order() == 7);
////  assert(input[5]->error() == 1);
////  assert(input[5]->order() == 1);
////  assert(input[6]->error() == 0);
////  assert(input[6]->order() == 5);
////  assert(input[7]->error() == 0);
////  assert(input[7]->order() == 3);
//
//}
////
//void FifoExecuterLowerBound::overlapTest4() {
//  Operations ops;
//
//  Operation* input[6];
//
//  Operation i_0(3, 7, 7, Operation::INSERT, 2);
//  Operation i_1(6, 9, 9, Operation::INSERT, 3);
//  Operation i_2(10, 12, 12, Operation::INSERT, 1);
//
//  Operation r_0(1, 11, 11, Operation::REMOVE, 1);
//  Operation r_1(2, 8, 8, Operation::REMOVE, 3);
//  Operation r_2(4, 5, 5, Operation::REMOVE, 2);
//
//  input[0] = &r_0;
//  input[1] = &r_1;
//  input[2] = &i_0;
//  input[3] = &r_2;
//  input[4] = &i_1;
//  input[5] = &i_2;
//
//  ops.Initialize(input, 6, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(i_0.lateness() == 0);
//  assert(i_1.lateness() == 0);
//  assert(i_2.lateness() == 0);
//  assert(i_0.age() == 0);
//  assert(i_1.age() == 0);
//  assert(i_2.age() == 0);
//
//  assert(r_0.lateness() == 2);
//  assert(r_1.lateness() == 1);
//  assert(r_2.lateness() == 0);
//  assert(r_0.age() == 0);
//  assert(r_1.age() == 1);
//  assert(r_2.age() == 2);
//
//  assert(i_0.lin_order() == 0);
//  assert(i_1.lin_order() == 2);
//  assert(i_2.lin_order() == 4);
//
//  assert(r_0.lin_order() == 5);
//  assert(r_1.lin_order() == 3);
//  assert(r_2.lin_order() == 1);
//
//  assert(i_0.error() == 0);
//  assert(i_0.order() == 0);
//  assert(i_1.error() == 0);
//  assert(i_1.order() == 2);
//  assert(i_2.error() == 0);
//  assert(i_2.order() == 4);
//
//  assert(r_0.error() == 0);
//  assert(r_0.order() == 5);
//  assert(r_1.error() == 0);
//  assert(r_1.order() == 3);
//  assert(r_2.error() == 0);
//  assert(r_2.order() == 1);
//}
//
//void FifoExecuterLowerBound::overlapTest5() {
//  Operations ops;
//
//  Operation* input[4];
//
//  Operation i_0(1, 8, 8, Operation::INSERT, 1);
//  input[0] = &i_0;
//
//  Operation i_1(5, 7, 7, Operation::INSERT, 2);
//  input[1] = &i_1;
//
//  Operation r_0(3, 4, 4, Operation::REMOVE, 1);
//  input[2] = &r_0;
//
//  Operation r_1(2, 6, 6, Operation::REMOVE, 2);
//  input[3] = &r_1;
//
//  ops.Initialize(input, 4, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(i_0.lin_order() == 0);
//  assert(i_1.lin_order() == 2);
//  assert(r_0.lin_order() == 1);
//  assert(r_1.lin_order() == 3);
//
//  assert(i_0.lateness() == 0);
//  assert(i_1.lateness() == 0);
//  assert(i_0.age() == 0);
//  assert(i_1.age() == 0);
//
//  assert(r_0.lateness() == 0);
//  assert(r_1.lateness() == 1);
//  assert(r_0.age() == 1);
//  assert(r_1.age() == 0);
//
//}
//
//void FifoExecuterLowerBound::overlapTest6() {
//  Operations ops;
//
//  Operation* input[8];
//
//  Operation i_0(0, 10, 10, Operation::INSERT, 1);
//  Operation i_1(2, 10, 10, Operation::INSERT, 2);
//  Operation i_2(4, 10, 10, Operation::INSERT, 3);
//  Operation i_3(6, 10, 10, Operation::INSERT, 4);
//
//  input[0] = &i_0;
//  input[1] = &i_1;
//  input[2] = &i_2;
//  input[3] = &i_3;
//
//  Operation r_0(20, 21, 21, Operation::REMOVE, 4);
//  Operation r_1(22, 23, 23, Operation::REMOVE, 3);
//  Operation r_2(24, 25, 25, Operation::REMOVE, 2);
//  Operation r_3(26, 27, 27, Operation::REMOVE, 1);
//
//  input[4] = &r_0;
//  input[5] = &r_1;
//  input[6] = &r_2;
//  input[7] = &r_3;
//
//  ops.Initialize(input, 8, true);
//
//  ops.CalculateOverlaps();
//  FifoExecuterLowerBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//  fifo.calculate_order();
//  fifo.calculate_op_fairness();
//
//  assert(i_0.lateness() == 3);
//  assert(i_1.lateness() == 2);
//  assert(i_2.lateness() == 1);
//  assert(i_3.lateness() == 0);
//  assert(i_0.age() == 0);
//  assert(i_1.age() == 1);
//  assert(i_2.age() == 2);
//  assert(i_3.age() == 3);
//
//}
//
////void FifoExecuterLowerBound::insertOverlapTest1() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 10, Operation::INSERT, 1);
////  input[0] = &i_0;
////  Operation i_1(2, 10, Operation::INSERT, 2);
////  input[1] = &i_1;
////  Operation i_2(4, 10, Operation::INSERT, 3);
////  input[2] = &i_2;
////  Operation i_3(6, 10, Operation::INSERT, 4);
////  input[3] = &i_3;
////
////  Operation r_0(11, 12, Operation::REMOVE, 4);
////  input[4] = r_0;
////  Operation r_1(13, 14, Operation::REMOVE, 3);
////  input[5] = r_1;
////  Operation r_2(15, 16, Operation::REMOVE, 2);
////  input[6] = r_2;
////  Operation r_3(17, 18, Operation::REMOVE, 1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 6);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 4);
////  assert(input[2].error() == 0);
////  assert(input[2].order() == 2);
////  assert(input[3].error() == 0);
////  assert(input[3].order() == 0);
////  assert(input[4].error() == 0);
////  assert(input[4].order() == 1);
////  assert(input[5].error() == 0);
////  assert(input[5].order() == 3);
////  assert(input[6].error() == 0);
////  assert(input[6].order() == 5);
////  assert(input[7].error() == 0);
////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapTest2() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 3, Operation::INSERT, 1);
////  input[0] = &i_0;
////  Operation i_1(2, 5, Operation::INSERT, 2);
////  input[1] = &i_1;
////  Operation i_2(4, 7, Operation::INSERT, 3);
////  input[2] = &i_2;
////  Operation i_3(6, 9, Operation::INSERT, 4);
////  input[3] = &i_3;
////
////  Operation r_0(11, 12, Operation::REMOVE, 2);
////  input[4] = &r_0;
////  Operation r_1(13, 14, Operation::REMOVE, 4);
////  input[5] = &r_1;
////  Operation r_2(15, 16, Operation::REMOVE, 3);
////  input[6] = &r_2;
////  Operation r_3(17, 18, Operation::REMOVE, 1);
////  input[7] = &r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 6);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 0);
////  assert(input[2].error() == 0);
////  assert(input[2].order() == 4);
////  assert(input[3].error() == 0);
////  assert(input[3].order() == 2);
////  assert(input[4].error() == 0);
////  assert(input[4].order() == 1);
////  assert(input[5].error() == 1);
////  assert(input[5].order() == 3);
////  assert(input[6].error() == 1);
////  assert(input[6].order() == 5);
////  assert(input[7].error() == 0);
////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest1() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 3, Operation::INSERT, 1);
////  input[0] = &i_0;
////  Operation i_1(2, 5, Operation::INSERT, 2);
////  input[1] = &i_1;
////  Operation i_2(4, 7, Operation::INSERT, 3);
////  input[2] = &i_2;
////  Operation i_3(6, 9, Operation::INSERT, 4);
////  input[3] = &i_3;
////
////  Operation r_0(10, 13, Operation::REMOVE, 4);
////  input[4] = &r_0;
////  Operation r_1(12, 15, Operation::REMOVE, 3);
////  input[5] = &r_1;
////  Operation r_2(14, 17, Operation::REMOVE, 2);
////  input[6] = &r_2;
////  Operation r_3(16, 19, Operation::REMOVE, 1);
////  input[7] = &r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 6);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 4);
////  assert(input[2].error() == 0);
////  assert(input[2].order() == 0);
////  assert(input[3].error() == 0);
////  assert(input[3].order() == 2);
////  assert(input[4].error() == 2);
////  assert(input[4].order() == 3);
////  assert(input[5].error() == 1);
////  assert(input[5].order() == 1);
////  assert(input[6].error() == 0);
////  assert(input[6].order() == 5);
////  assert(input[7].error() == 0);
////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest2() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 3, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation i_1(2, 5, Operation::INSERT, 2);
////  input[1] = i_1;
////  Operation r_0(2, 6, Operation::REMOVE, 2);
////  input[2] = r_0;
////  Operation r_1(4, 7, Operation::REMOVE, 1);
////  input[3] = r_1;
////  Operation i_2(10, 17, Operation::INSERT, 3);
////  input[4] = i_2;
////  Operation i_3(16, 19, Operation::INSERT, 4);
////  input[5] = i_3;
////  Operation r_2(16, 19, Operation::REMOVE, 4);
////  input[6] = r_2;
////  Operation r_3(118, 19, Operation::REMOVE, 3);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 2);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 0);
////  assert(input[4].error() == 0);
////  assert(input[4].order() == 6);
////  assert(input[5].error() == 0);
////  assert(input[5].order() == 4);
////  assert(input[2].error() == 0);
////  assert(input[2].order() == 1);
////  assert(input[3].error() == 0);
////  assert(input[3].order() == 3);
////  assert(input[6].error() == 0);
////  assert(input[6].order() == 5);
////  assert(input[7].error() == 0);
////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest3() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 20, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation r_0(1, 18, Operation::REMOVE, 4);
////  input[1] = r_0;
////  Operation r_1(4, 14, Operation::REMOVE, 3);
////  input[2] = r_1;
////  Operation i_1(5, 15, Operation::INSERT, 2);
////  input[3] = i_1;
////  Operation r_2(6, 11, Operation::REMOVE, 2);
////  input[4] = r_2;
////  Operation i_2(7, 12, Operation::INSERT, 3);
////  input[5] = i_2;
////  Operation i_3(10, 11, Operation::INSERT, 4);
////  input[6] = i_3;
////  Operation r_3(10, 11, Operation::REMOVE, 1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  for(int i = 0; i < 8; i++) {
////    assert(input[i].error() == 0);
////  }
////
//////  assert(input[0].error() == 0);
//////  assert(input[0].order() == 6);
//////  assert(input[3].error() == 0);
//////  assert(input[3].order() == 4);
//////  assert(input[5].error() == 0);
//////  assert(input[5].order() == 2);
//////  assert(input[6].error() == 0);
//////  assert(input[6].order() == 0);
//////  assert(input[1].error() == 0);
//////  assert(input[1].order() == 1);
//////  assert(input[2].error() == 0);
//////  assert(input[2].order() == 3);
//////  assert(input[4].error() == 0);
//////  assert(input[4].order() == 5);
//////  assert(input[7].error() == 0);
//////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest4() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 14, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation r_0(1, 12, Operation::REMOVE, 3);
////  input[1] = r_0;
////  Operation i_1(4, 16, Operation::INSERT, 2);
////  input[2] = i_1;
////  Operation r_1(6, 16, Operation::REMOVE, 4);
////  input[3] = r_1;
////  Operation i_2(8, 18, Operation::INSERT, 3);
////  input[4] = i_2;
////  Operation r_2(13, 16, Operation::REMOVE, 2);
////  input[5] = r_2;
////  Operation i_3(15, 19, Operation::INSERT, 4);
////  input[6] = i_3;
////  Operation r_3(17, 19, Operation::REMOVE, 1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  for(int i = 0; i < 8; i++) {
////    if(input[i].error() == 1) {
////      assert(input[i].value() == 4);
////    }
////  }
////
//////  assert(input[0].error() == 0);
//////  assert(input[0].order() == 6);
//////  assert(input[2].error() == 0);
//////  assert(input[2].order() == 2);
//////  assert(input[4].error() == 0);
//////  assert(input[4].order() == 0);
//////  assert(input[6].error() == 0);
//////  assert(input[6].order() == 4);
//////  assert(input[1].error() == 0);
//////  assert(input[1].order() == 1);
//////  assert(input[3].error() == 1);
//////  assert(input[3].order() == 5);
//////  assert(input[5].error() == 0);
//////  assert(input[5].order() == 3);
//////  assert(input[7].error() == 0);
//////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest5() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 20, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation r_0(1, 20, Operation::REMOVE, 3);
////  input[1] = r_0;
////  Operation i_1(4, 10, Operation::INSERT, 2);
////  input[2] = i_1;
////  Operation r_1(6, 12, Operation::REMOVE, 2);
////  input[3] = r_1;
////  Operation i_2(8, 14, Operation::INSERT, 3);
////  input[4] = i_2;
////  Operation r_2(10, 14, Operation::REMOVE, 4);
////  input[5] = r_2;
////  Operation i_3(12, 20, Operation::INSERT, 4);
////  input[6] = i_3;
////  Operation r_3(16, 20, Operation::REMOVE, 1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  for(int i = 0; i < 8; i++) {
////    assert(input[i].error() == 0);
////  }
////
//////  assert(input[0].error() == 0);
//////  assert(input[0].order() == 6);
//////  assert(input[2].error() == 0);
//////  assert(input[2].order() == 2);
//////  assert(input[4].error() == 0);
//////  assert(input[4].order() == 0);
//////  assert(input[6].error() == 0);
//////  assert(input[6].order() == 4);
//////  assert(input[1].error() == 0);
//////  assert(input[1].order() == 1);
//////  assert(input[3].error() == 0);
//////  assert(input[3].order() == 3);
//////  assert(input[5].error() == 0);
//////  assert(input[5].order() == 5);
//////  assert(input[7].error() == 0);
//////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::insertOverlapMixedTest6() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 6, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation i_1(4, 10, Operation::INSERT, 2);
////  input[1] = i_1;
////  Operation r_0(4, 10, Operation::REMOVE, 3);
////  input[2] = r_0;
////  Operation i_2(8, 14, Operation::INSERT, 3);
////  input[3] = i_2;
////  Operation r_1(8, 14, Operation::REMOVE, 4);
////    input[4] = r_1;
////  Operation i_3(12, 16, Operation::INSERT, 4);
////  input[5] = i_3;
////  Operation r_2(12, 16, Operation::REMOVE, 1);
////  input[6] = r_2;
////  Operation r_3(14, 18, Operation::REMOVE, 2);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 2);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 6);
////  assert(input[3].error() == 0);
////  assert(input[3].order() == 0);
////  assert(input[5].error() == 0);
////  assert(input[5].order() == 4);
////  assert(input[2].error() == 1);
////  assert(input[2].order() == 1);
////  assert(input[4].error() == 1);
////  assert(input[4].order() == 5);
////  assert(input[6].error() == 0);
////  assert(input[6].order() == 3);
////  assert(input[7].error() == 0);
////  assert(input[7].order() == 7);
////}
////
////void FifoExecuterLowerBound::removeNullTest1() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 6, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation i_1(4, 10, Operation::INSERT, 2);
////  input[1] = i_1;
////  Operation r_0(4, 10, Operation::REMOVE, -1);
////  input[2] = r_0;
////  Operation i_2(8, 14, Operation::INSERT, 3);
////  input[3] = i_2;
////  Operation r_1(8, 14, Operation::REMOVE, 1);
////  input[4] = r_1;
////  Operation i_3(12, 16, Operation::INSERT, 4);
////  input[5] = i_3;
////
////
////
////  Operation r_2(12, 16, Operation::REMOVE, 2);
////  input[6] = r_2;
////  Operation r_3(17, 18, Operation::REMOVE, -1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  assert(input[0].error() == 0);
////  assert(input[0].order() == 1);
////  assert(input[1].error() == 0);
////  assert(input[1].order() == 3);
////  assert(input[2].error() == 0);
////  assert(input[2].order() == 0);
////  assert(input[4].error() == 0);
////  assert(input[4].order() == 2);
////  assert(input[6].error() == 0);
////  assert(input[6].order() == 4);
////  assert(input[7].error() == 2);
////  assert(input[7].order() == 5);
////}
////
////void FifoExecuterLowerBound::removeNullTest2() {
////  Operations ops;
////
////  Operation* input[8];
////
////  Operation i_0(0, 6, Operation::INSERT, 1);
////  input[0] = i_0;
////  Operation i_1(4, 10, Operation::INSERT, 2);
////  input[1] = i_1;
////  Operation r_0(4, 10, Operation::REMOVE, 3);
////  input[2] = r_0;
////  Operation r_1(5, 14, Operation::REMOVE, -1);
////  input[3] = r_1;
////  Operation i_2(8, 14, Operation::INSERT, 3);
////  input[4] = i_2;
////  Operation r_2(9, 16, Operation::REMOVE, -1);
////  input[5] = r_2;
////  Operation i_3(12, 16, Operation::INSERT, 4);
////  input[6] = i_3;
////  Operation r_3(14, 18, Operation::REMOVE, -1);
////  input[7] = r_3;
////
////  ops.Initialize(input, 8);
////
////  ops.CalculateOverlaps();
////  FifoExecuterLowerBound fifo(&ops);
////  Histogram histogram;
////  fifo.execute(&histogram);
////
////  for(int i = 0; i < 8; i++) {
////    if(input[i].error() == 2) {
////      assert(input[i].start() == 14);
////    } else if(input[i].error() == 1) {
////      assert(input[i].start() == 9 || input[i].value() == 3);
////    }
////  }
////
//////  assert(input[4].error() == 0);
//////  assert(input[4].order() == 0);
//////  assert(input[2].error() == 1);
//////  assert(input[2].order() == 1);
//////  assert(input[3].error() == 0);
//////  assert(input[3].order() == 2);
//////  assert(input[5].error() == 1);
//////  assert(input[5].order() == 3);
//////  assert(input[7].error() == 2);
//////  assert(input[7].order() == 4);
////
////}
//
//void FifoExecuterLowerBound::test() {
//
//  overlapTest3();
//  overlapTest1();
//  overlapTest2();
////  insertOverlapTest1();
////  insertOverlapTest2();
////  insertOverlapMixedTest3();
////  insertOverlapMixedTest1();
////  insertOverlapMixedTest2();
////  insertOverlapMixedTest4();
////  insertOverlapMixedTest5();
////  insertOverlapMixedTest6();
////  removeNullTest1();
////  removeNullTest2();
//  overlapTest4();
//  overlapTest5();
//  overlapTest6();
//}
