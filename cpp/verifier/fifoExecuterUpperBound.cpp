
#include "fifoExecuterUpperBound.h"
#include "histogram.h"

#include <assert.h>
// go until last overlapping insert
int FifoExecuterUpperBound::semanticalError(const Operation* removeOperation,
                                Operation** matchingInsertOperation) const {

  int semantical_error = 0;
  Operation *match = NULL;
  Operations::Iterator iter = ops_->elements();
  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {
    if (match != NULL && element->start() > match->end()) {
      break;
    }
    if (element->type() == Operation::INSERT) {
      if (element->value() == removeOperation->value()) {  //TODO: same value case needs some re-thinking
        match = element;
      } else {
        semantical_error++;
      }
    }
  }
  if (match != NULL) {
    if (matchingInsertOperation != NULL) {
      *matchingInsertOperation = match;
    }
    return semantical_error;
  }
  return -1;
}

void FifoExecuterUpperBound::executeOperationWithOverlaps(Operation* element, Histogram* histogram) {
  int error;
  Operation* matchingInsertOperation;
  bool null_return = (element->value() == -1);

  if (null_return) {
    error = amountOfStartedEnqueueOperations(element);
    histogram->add(error);
    ops_->remove(element, error);
    return;
  }

  error = semanticalError(element, &matchingInsertOperation);
  if (error == -1) {
    fprintf(stderr, "FATAL2: errorDistance -1, element->value %d\n", element->value());
    exit(-1);
  }

  while (true) {
    vector<ErrorEntry> overlaps;
    element->determineExecutionOrderUpperBound(*ops_, this, error, &overlaps);
    if (overlaps.size() == 0) {
      break;
    }
    // TODO: CONTINUE!!!
    for (size_t i = 0; i < overlaps.size(); i++) {
      assert(overlaps[i].error_ >= error);        
      histogram->add(overlaps[i].error_);
      if (overlaps[i].matching_insert_ != NULL) {
        ops_->remove(overlaps[i].matching_insert_, 0);
      }
      ops_->remove(overlaps[i].op_, overlaps[i].error_);
    }
  }

  error = semanticalError(element, &matchingInsertOperation);//not necessary?
  histogram->add(error);
  ops_->remove(matchingInsertOperation, 0);
  ops_->remove(element, error);
  return;
}

//void FifoExecuterUpperBound::noOverlapTest1() {
//  Operations ops;
//
//  Operation input[8];
//
//  for (int i = 0; i < 4; ++i) {
//    Operation element(i*2, i*2 + 1, Operation::INSERT, i+1);
//    input[i] = element;
//  }
//
//  Operation r_0(10, 11, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(12, 13, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(14, 15, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(16, 17, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//
//void FifoExecuterUpperBound::overlapTest1() {
//  Operations ops;
//
//  Operation input[8];
//
//  for (int i = 0; i < 4; ++i) {
//    Operation element(i*2, i*2 + 1, Operation::INSERT, i+1);
//    input[i] = element;
//  }
//
//  Operation r_0(10, 20, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(12, 13, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(14, 15, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(16, 17, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::overlapTest2() {
//  Operations ops;
//
//  Operation input[8];
//
//  for (int i = 0; i < 4; ++i) {
//    Operation element(i*2, i*2 + 1, Operation::INSERT, i+1);
//    input[i] = element;
//  }
//
//  Operation r_0(10, 13, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(12, 15, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(14, 17, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(16, 19, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//void FifoExecuterUpperBound::overlapTest3() {
//
//  Operations ops;
//  Operation input[6];
//
//  Operation r_0(1, 2, Operation::INSERT, 1);
//  input[0] = r_0;
//  Operation r_1(3, 5, Operation::INSERT, 2);
//  input[1] = r_1;
//  Operation r_2(4, 6, Operation::INSERT, 3);
//  input[2] = r_2;
//
//  Operation r_3(7, 12, Operation::REMOVE, 3);
//  input[3] = r_3;
//  Operation r_4(8, 9, Operation::REMOVE, 2);
//  input[4] = r_4;
//  Operation r_5(10, 11, Operation::REMOVE, 1);
//  input[5] = r_5;
//
//  ops.Initialize(input, 6);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  for (int i = 0; i < 6; i++) {
//    printf("Upper: Operation: %d; Element %d; order: %d; error: %d \n", input[i].type(), input[i].value(), input[i].order(), input[i].error());
//  }
//}
//
//void FifoExecuterUpperBound::insertOverlapTest1() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 10, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(2, 10, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(4, 10, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(6, 10, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(11, 12, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(13, 14, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(15, 16, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(17, 18, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapTest2() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 3, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(2, 5, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(4, 7, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(6, 9, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(11, 12, Operation::REMOVE, 2);
//  input[4] = r_0;
//  Operation r_1(13, 14, Operation::REMOVE, 4);
//  input[5] = r_1;
//  Operation r_2(15, 16, Operation::REMOVE, 3);
//  input[6] = r_2;
//  Operation r_3(17, 18, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 0);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 4);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 2);
//  assert(input[4].error() == 2);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest1() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 3, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(2, 5, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(4, 7, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(6, 9, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(10, 13, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(12, 15, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(14, 17, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(16, 19, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest2() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 3, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(2, 5, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(10, 17, Operation::INSERT, 3);
//  input[4] = i_2;
//  Operation i_3(16, 19, Operation::INSERT, 4);
//  input[5] = i_3;
//
//  Operation r_0(2, 6, Operation::REMOVE, 2);
//  input[2] = r_0;
//  Operation r_1(4, 7, Operation::REMOVE, 1);
//  input[3] = r_1;
//  Operation r_2(16, 19, Operation::REMOVE, 4);
//  input[6] = r_2;
//  Operation r_3(18, 19, Operation::REMOVE, 3);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 2);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 0);
//  assert(input[4].error() == 0);
//  assert(input[4].order() == 6);
//  assert(input[5].error() == 0);
//  assert(input[5].order() == 4);
//  assert(input[2].error() == 1);
//  assert(input[2].order() == 1);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest3() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 20, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(5, 15, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(7, 12, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(10, 11, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(1, 18, Operation::REMOVE, 4);
//  input[4] = r_0;
//  Operation r_1(4, 14, Operation::REMOVE, 3);
//  input[5] = r_1;
//  Operation r_2(6, 11, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(10, 11, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 0);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest4() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 14, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(4, 16, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(8, 18, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(15, 19, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(1, 12, Operation::REMOVE, 3);
//  input[4] = r_0;
//  Operation r_1(6, 16, Operation::REMOVE, 4);
//  input[5] = r_1;
//  Operation r_2(13, 16, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(17, 19, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
///*  int i;
//  for (i=0; i<8; i++) {
//    printf("%d %d\n", input[i].error(), input[i].order());
//  }*/
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 4);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 0);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 2);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest5() {
//  Operations ops;
//  Operation input[8];
//
//  Operation i_0(0, 20, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(4, 10, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(8, 14, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(12, 20, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(1, 20, Operation::REMOVE, 3);
//  input[4] = r_0;
//  Operation r_1(6, 12, Operation::REMOVE, 2);
//  input[5] = r_1;
//  Operation r_2(10, 14, Operation::REMOVE, 4);
//  input[6] = r_2;
//  Operation r_3(16, 20, Operation::REMOVE, 1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 6);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 2);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 0);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 4);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 1);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::insertOverlapMixedTest6() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 6, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(4, 10, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(8, 14, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(12, 16, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(4, 10, Operation::REMOVE, 3);
//  input[4] = r_0;
//  Operation r_1(8, 14, Operation::REMOVE, 4);
//  input[5] = r_1;
//  Operation r_2(12, 16, Operation::REMOVE, 1);
//  input[6] = r_2;
//  Operation r_3(14, 18, Operation::REMOVE, 2);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 4);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 6);
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 0);
//  assert(input[3].error() == 0);
//  assert(input[3].order() == 2);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 1);
//  assert(input[5].error() == 2);
//  assert(input[5].order() == 3);
//  assert(input[6].error() == 1);
//  assert(input[6].order() == 5);
//  assert(input[7].error() == 0);
//  assert(input[7].order() == 7);
//}
//
//void FifoExecuterUpperBound::removeNullTest1(){
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 6, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(4, 10, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(8, 14, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(12, 16, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(4, 10, Operation::REMOVE, -1);
//  input[4] = r_0;
//  Operation r_1(8, 14, Operation::REMOVE, 1);
//  input[5] = r_1;
//  Operation r_2(12, 16, Operation::REMOVE, 2);
//  input[6] = r_2;
//  Operation r_3(17, 18, Operation::REMOVE, -1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[0].error() == 0);
//  assert(input[0].order() == 3);
//  assert(input[1].error() == 0);
//  assert(input[1].order() == 1);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 0);
//  assert(input[5].error() == 0);
//  assert(input[5].order() == 4);
//  assert(input[6].error() == 2);
//  assert(input[6].order() == 2);
//  assert(input[7].error() == 2);
//  assert(input[7].order() == 5);
//}
//
//void FifoExecuterUpperBound::removeNullTest2() {
//  Operations ops;
//
//  Operation input[8];
//
//  Operation i_0(0, 6, Operation::INSERT, 1);
//  input[0] = i_0;
//  Operation i_1(4, 10, Operation::INSERT, 2);
//  input[1] = i_1;
//  Operation i_2(8, 14, Operation::INSERT, 3);
//  input[2] = i_2;
//  Operation i_3(12, 16, Operation::INSERT, 4);
//  input[3] = i_3;
//
//  Operation r_0(4, 10, Operation::REMOVE, 3);
//  input[4] = r_0;
//  Operation r_1(5, 14, Operation::REMOVE, -1);
//  input[5] = r_1;
//  Operation r_2(9, 16, Operation::REMOVE, -1);
//  input[6] = r_2;
//  Operation r_3(14, 18, Operation::REMOVE, -1);
//  input[7] = r_3;
//
//  ops.Initialize(input, 8);
//
//  ops.CalculateOverlaps();
//  FifoExecuterUpperBound fifo(&ops);
//  Histogram histogram;
//  fifo.execute(&histogram);
//
//  assert(input[2].error() == 0);
//  assert(input[2].order() == 2);
//  assert(input[4].error() == 3);
//  assert(input[4].order() == 3);
//  assert(input[5].error() == 4);
//  assert(input[5].order() == 0);
//  assert(input[6].error() == 4);
//  assert(input[6].order() == 1);
//  assert(input[7].error() == 3);
//  assert(input[7].order() == 4);
//}

void FifoExecuterUpperBound::test() {
//  noOverlapTest1();
//  overlapTest1();
//  overlapTest2();
////  overlapTest3();
//  insertOverlapTest1();
//  insertOverlapTest2();
//  insertOverlapMixedTest1();
//  insertOverlapMixedTest2();
//  insertOverlapMixedTest3();
//  insertOverlapMixedTest4();
//  insertOverlapMixedTest5();
//  insertOverlapMixedTest6();
//  removeNullTest1();
//  removeNullTest2();
}
