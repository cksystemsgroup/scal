#ifndef OPERATION_H
#define OPERATION_H

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <assert.h>

#include "histogram.h"

using std::vector;

// What is the type of time?
typedef uint64_t Time;

class FifoExecuter;
class FifoExecuterLowerBound;
class Operations;
class Operation;

class ErrorEntry {
 public:
  ErrorEntry(Operation* op, int error, Operation* matching_insert) : op_(op), error_(error), matching_insert_(matching_insert) {}
  Operation* op_;
  int error_;
  Operation* matching_insert_;
};

class Operation {
  public:
    enum OperationType {
      INSERT,
      REMOVE,
    };

    Operation();
    Operation(Time start, Time end);
    Operation(Time start, Time end, int id);
    Operation(Time start, Time end, OperationType type, int value);

    Time start() const { return start_; }
    Time end() const { return end_; }
    int id() const { return id_; }
    OperationType type() const { return type_; }
    int value() const { return value_; }
    bool deleted() const { return deleted_; }
    int order() const { return order_; }
    int error() const { return error_; }

    void remove() {
      next_->prev_ = prev_;
      prev_->next_ = next_;
      deleted_ = true;
    }

    void reinsert() {
      deleted_ = false;
      next_->prev_ = this;
      prev_->next_ = this;
    }

    void print() const;

    // Insert op just before this element.
    void insert_before(Operation *op);

    // Returns true if op overlaps with this.
    inline bool overlaps_insert(const Operation* op) const {
      return std::binary_search(insert_overlaps_value_.begin(), insert_overlaps_value_.end(), op->value());
    }
    // Returns true if op overlaps with this.
    inline bool overlaps_remove(const Operation* op) const {
      return std::binary_search(remove_overlaps_value_.begin(), remove_overlaps_value_.end(), op->value());
    }
    void determineExecutionOrderLowerBound(Operations& ops,
                               FifoExecuter* executer,
                               int errorDistance,
                               vector<ErrorEntry>* result) const;

    void determineExecutionOrderUpperBound(Operations& ops,
                               FifoExecuter* executer,
                               int errorDistance,
                               vector<ErrorEntry>* result) const;

    // Overload comparison operators.
    bool operator<(const Operation& op) const;
    bool operator<=(const Operation& op) const;
    bool operator>(const Operation& op) const;
    bool operator>=(const Operation& op) const;

    // Unit test this class.
    static void test();
  private:
    Time start_;
    Time real_start_;
    Time end_;
    int id_;
    OperationType type_;
    int value_;
    Operation* next_;
    Operation* prev_;
    vector<int> overlaps_;
    vector<int> insert_overlaps_value_;
    vector<int> remove_overlaps_value_;
    bool deleted_;
    int order_;
    int error_;
    friend class Operations;
};


class Operations {
  public:
    Operations() : head_(0, 0, 0) {
      head_.deleted_ = false;
      execution_id_ = 0;
    }
    Operations(FILE* input, int num_ops);
    void Initialize(Operation** ops, int num_ops);
    const Operation** operator[](int i) const;
    Operation** operator[](int i);
    bool is_consistent();
    void print();

    Operation* begin() { return head_.next_; }
    void remove(Operation* op, int error) { op->remove(); op->error_ = error; op->order_ = execution_id_++; }
    bool is_empty() const { return head_.next_ == &head_; }
    bool is_single_element() const { return !is_empty() && (head_.next_ == head_.prev_); }

    void CalculateOverlaps();

    class OverlapIterator {
      public:
        OverlapIterator(Operations* set) : set_(set), next_(set->head_.next_) {
          from_ = next_->start();
          until_ = next_->end();
        }
        bool has_next() const;
        Operation* get_next();

      private:
        Time from_;
        Time until_;
        Operations* set_;
        Operation* next_;
    };

    class Iterator {
     public:
      Iterator(Operations* ops) : ops_(ops), next_(ops->head_.next_) {
      }
      bool valid() const {
        return next_ != &ops_->head_;
      }
      Operation* step() {
        next_ = next_->next_;
        assert(!next_->deleted());
        return next_;
      }
      Operation* get() const {
        assert(!next_->deleted());
        return next_;
      }
     private:
      const Operations* ops_;
      Operation* next_;
    };

    Iterator elements() {
      return Iterator(this);
    }

    OverlapIterator* Overlaps() {
      return new OverlapIterator(this);
    }

    static void test();
  private:
    void insert(Operation* op);
    void adjust_start_times(Operation** ops, int num_operations);

  // All operations as we read them.
    Operation** ops_;
    int num_operations_;
    // Operations sorted by invocation.
    Operation head_;
    int execution_id_;
};
#endif  // OPERATION_H
