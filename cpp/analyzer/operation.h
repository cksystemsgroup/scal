#ifndef OPERATION_H
#define OPERATION_H

#include <stdint.h>

class Operation {

  public:
    enum OperationType {
      INSERT,
      REMOVE,
    };

    Operation();

    void initialize(uint64_t start, uint64_t end, OperationType type, uint64_t value);

    uint64_t id() const {
      return id_;
    }

    uint64_t start() const {
      return start_;
    }

    uint64_t real_start() const {
      return real_start_;
    }

    uint64_t end() const {
      return end_;
    }

    uint64_t real_end() const {
      return real_end_;
    }

    OperationType type() const {
      return type_;
    }

    uint64_t value() const {
      return value_;
    }

  private:
    
    
    uint64_t id_;
    uint64_t start_;
    uint64_t real_start_;
    uint64_t end_;
    uint64_t real_end_;
    OperationType type_;
    uint64_t value_;
};

#endif // OPERATION_H
