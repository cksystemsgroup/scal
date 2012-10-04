#ifndef FIFO_EXECUTER_H
#define FIFO_EXECUTER_H

#include "operation.h"
#include "executer.h"

class Histogram;

class FifoExecuter : public Executer {
 public:
  FifoExecuter(Operations* ops) : ops_(ops) {}
  virtual void execute(Histogram* histgram);
  virtual int semanticalError(const Operation* removeOperation,
                      Operation** matchingInsertOperation = NULL) const = 0;

  int amountOfFinishedEnqueueOperations(Operation* removeOperation) const;
  int amountOfStartedEnqueueOperations(Operation* removeOperation) const;
  void calculate_order();
  void calculate_response_order();
  int calculate_op_fairness();

 protected:
  virtual void executeOperationWithOverlaps(Operation* element, Histogram* histogram) = 0;

  void executeOperation(Operation* element, Histogram* histogram);

  Operations* ops_;
};

#endif  // FIFO_EXECUTER_H
