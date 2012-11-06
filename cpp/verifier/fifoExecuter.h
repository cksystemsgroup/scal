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
  void calculate_element_fairness();
  void calculate_new_element_fairness(uint64_t (*lin_point)(Operation* op), int (*compare)(const void* left, const void* right));
  void calculate_diff(uint64_t (*lin_point_1)(Operation* op), int (*compare_ops_1)(const void* left, const void* right),
                      uint64_t (*lin_point_2)(Operation* op), int (*compare_ops_2)(const void* left, const void* right));
  void aggregate_semantical_error();
  void calculate_op_fairness();
  void calculate_op_fairness_typeless();
  void calculate_performance_index();


 protected:
  virtual void executeOperationWithOverlaps(Operation* element, Histogram* histogram) = 0;

  void executeOperation(Operation* element, Histogram* histogram);

  Operations* ops_;

private:
  void mark_long_ops(Operation** upper_bounds, bool* ignore_flags);
  void calculate_upper_bounds(bool* ignore_flags, Operation** upper_bounds);
};

#endif  // FIFO_EXECUTER_H
