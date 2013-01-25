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

    void initialize(uint64_t start, uint64_t lin_time, uint64_t end, OperationType type, uint64_t value);

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

    uint64_t lin_time() const {
      return lin_time_;
    }

    OperationType type() const {
      return type_;
    }

    int64_t value() const {
      return value_;
    }

    Operation* matching() const {
      return matching_;
    }

  private:
    
    // A unique ID.
    uint64_t id_;
    // The start time of the operation, possibly adjusted.
    uint64_t start_;
    // The start time of the operation, as it was in the logfile.
    uint64_t real_start_;
    // The end time of the operation, possibly adjusted.
    uint64_t end_;
    // The end time of the operation, as it was in the logfile.
    uint64_t real_end_;
    // The timestamp of the linearization point.
    uint64_t lin_time_;
    // The type of the operation, either enqueue or dequeue.
    OperationType type_;
    // The element that was inserted or removed by the operation.
    // The value -1 indicates a null-return.
    int64_t value_;
    // The enqueue or dequeue operation which has the same value as this
    // operation.
    Operation* matching_;
};

#endif // OPERATION_H
