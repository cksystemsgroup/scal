
#include "fifoExecuter.h"
#include "histogram.h"

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
