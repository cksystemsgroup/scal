#ifndef EXECUTER_H
#define EXECUTER_H

class Histogram;
class Operation;

class Executer {
 public:
  virtual void execute(Histogram* histgram) = 0;
};

#endif  // EXECUTER_H
