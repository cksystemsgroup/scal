#include "semantic.h"


Traces* NullSemantic::prune(Traces* traces, Operation* op) {
  return new Traces(*traces);
}

FifoSemantic::FifoSemantic(int k, Operations* operations) 
    : operations_(operations), k_(k) {

}

bool FifoSemantic::check(Trace* trace, Operation* op) {
  int error = 0;
  for (size_t i = 0; i < trace->size(); i++) {
    Operation* t_op = *(*operations_)[trace->id(i)];
    if ((t_op->type() == Operation::INSERT) &&
        (trace->flag(i) != MATCHED)) {
      if (t_op->value() == op->value()) {
        trace->set_flag(i, MATCHED);
        return error < k_;
      }
      error++;
    }
  }
  return false;
}

Traces* FifoSemantic::prune(Traces* traces, Operation* op) {
  Traces* pruned = new Traces();
  // look at all traces, and return a set of traces that adhere to fifo.
  if (op->type() == Operation::REMOVE) {
    for (Traces::Iterator it = traces->begin(); it != traces->end(); it++) {
      if (check(&(*it), op)) {
        pruned->append(*it);
      }
    }
  } else {
    *pruned = *traces;
  }

  return pruned;
}
