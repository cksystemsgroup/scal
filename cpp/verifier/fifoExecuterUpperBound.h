#ifndef FIFO_EXECUTER_UPPER_BOUND_H
#define FIFO_EXECUTER_UPPER_BOUND_H

#include "operation.h"
#include "fifoExecuter.h"

class Histogram;

class FifoExecuterUpperBound : public FifoExecuter {
 public:
  FifoExecuterUpperBound(Operations* ops) : FifoExecuter(ops) {}
  virtual int semanticalError(const Operation* removeOperation,
                      Operation** matchingInsertOperation = NULL) const;
  static void test();
 private:
  virtual void executeOperationWithOverlaps(Operation* element, Histogram* histogram);

//  static void noOverlapTest1();
//  static void overlapTest1();
//  static void overlapTest2();
//  static void overlapTest3();
//  static void insertOverlapTest1();
//  static void insertOverlapTest2();
//  static void insertOverlapMixedTest1();
//  static void insertOverlapMixedTest2();
//  static void insertOverlapMixedTest3();
//  static void insertOverlapMixedTest4();
//  static void insertOverlapMixedTest5();
//  static void insertOverlapMixedTest6();
//  static void removeNullTest1();
//  static void removeNullTest2();
};

#endif  // FIFO_EXECUTER_UPPER_BOUND_H
