#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "operation.h"
#include "trace.h"

class Semantic {
  public:
    virtual Traces* prune(Traces* traces, Operation* op) = 0;
};

class NullSemantic : public Semantic {
  public:
    NullSemantic() { }
    virtual Traces* prune(Traces* traces, Operation* op);
};

class FifoSemantic : public Semantic {
  public:
    FifoSemantic(int k, Operations* operations);
    virtual Traces* prune(Traces* traces, Operation* op);
  private:
    static const int MATCHED = 0xdeadbeef;
    bool check(Trace* trace, Operation* op);

    Operations* operations_;
    int k_;
};
#endif
