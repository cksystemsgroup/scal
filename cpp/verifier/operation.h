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
  ErrorEntry(Operation* op, int error, Operation* matching_insert) :
      op_(op), error_(error), matching_insert_(matching_insert) {
  }
  Operation* op_;
  int error_;
  Operation* matching_insert_;
};

class Operation {
public:
  enum OperationType {
    INSERT, REMOVE,
  };

  Operation();
  Operation(Time start, Time end);
  Operation(Time start, Time end, int id);
  Operation(Time start, Time end, OperationType type, int value);
  Operation(Time start, Time end, int id, OperationType type, int value);

  // Each insert operation stores a list of overlapping insert operations to calculate
  // the order.
  vector<Operation*> overlaps_ops_same_type_;

  Time start() const {
    return start_;
  }
  Time real_start() const {
    return real_start_;
  }
  Time real_end() const {
    return real_end_;
  }
  Time end() const {
    return end_;
  }
  int id() const {
    return id_;
  }
  OperationType type() const {
    return type_;
  }
  int value() const {
    return value_;
  }
  bool deleted() const {
    return deleted_;
  }
  int order() const {
    return order_;
  }
  int lin_order() const {
    return lin_order_;
  }
  unsigned int error() const {
    return error_;
  }
  void set_lin_order(int value) {
    lin_order_ = value;
  }
  void increment_age() {
    age_++;
  }
  void increment_lateness() {
    lateness_++;
  }
  int age() const {
    return age_;
  }
  int lateness() const {
    return lateness_;
  }
  Operation* matching_op() const {
    return matching_op_;
  }

  void set_overtakes(int value) {
    overtakes_ = value;
  }

  void set_real_start(int value) {
      real_start_ = value;
  }

  int overtakes() const{
    return overtakes_;
  }

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
//    inline bool overlaps_insert(const Operation* op) const {
//      return std::binary_search(insert_overlaps_value_.begin(), insert_overlaps_value_.end(), op->value());
//    }
  // Returns true if op overlaps with this.
//    inline bool overlaps_remove(const Operation* op) const {
//      return std::binary_search(remove_overlaps_value_.begin(), remove_overlaps_value_.end(), op->value());
//    }
  void determineExecutionOrderLowerBound(Operations& ops,
      FifoExecuter* executer, int errorDistance, vector<ErrorEntry>* result,
      Time* linearizable_time_window_end);

  void determineExecutionOrderUpperBound(Operations& ops,
      FifoExecuter* executer, int errorDistance,
      vector<ErrorEntry>* result) const;

  // Overload comparison operators.
  bool operator<(const Operation& op) const;
  bool operator<=(const Operation& op) const;
  bool operator>(const Operation& op) const;
  bool operator>=(const Operation& op) const;

  static uint64_t lin_point_start_time(Operation* op) {
    return op->start();
  }

  static uint64_t lin_point_end_time(Operation* op) {
      return op->end();
    }

  static int compare_operations_by_start_time(const void* left,
      const void* right) {
    return (*(Operation**) left)->start() > (*(Operation**) right)->start();
  }

  static int compare_operations_by_end_time(const void* left,
      const void* right) {
    return (*(Operation**) left)->end() > (*(Operation**) right)->end();
  }

  static int compare_operations_by_real_start_time(const void* left,
      const void* right) {
    return (*(Operation**) left)->real_start_
        > (*(Operation**) right)->real_start_;
  }

  static int compare_operations_by_order(const void* left, const void* right) {
    return (*(Operation**) left)->order() > (*(Operation**) right)->order();
  }

  static int compare_ops_by_value(const void* left, const void* right) {
    return (*(Operation**) left)->value() > (*(Operation**) right)->value();
  }

  // Unit test this class.
  static void test();
private:
  Time start_;
  Time real_start_;
  Time end_;
  Time real_end_;
  OperationType type_;
  int value_;
  Operation* next_;
  Operation* prev_;
  bool deleted_;
  unsigned int error_;
  int id_;
  int order_;
  Operation* matching_op_;
  vector<int> overlaps_;
//    vector<int> insert_overlaps_value_;
//    vector<int> remove_overlaps_value_;
  int lin_order_;
  int lateness_;
  int age_;
  int overtakes_;
  friend class Operations;
};

class Operations {
public:
  Operations() :
      head_(0, 0, 0) {
    head_.deleted_ = false;
    execution_id_ = 0;
    ops_ = NULL;
    insert_ops_ = NULL;
    remove_ops_ = NULL;
    num_operations_ = 0;
    num_insert_ops_ = 0;
    num_remove_ops_ = 0;
  }
  Operations(char* filename, int num_ops, bool adjust_start_times);
  void Initialize(Operation** ops, int num_ops, bool adjust_start_times);
  const Operation** operator[](int i) const;
  Operation** operator[](int i);
  bool is_consistent();
  void print();

  static void create_doubly_linked_list(Operation* head, Operation** ops,
      int num_operations) {

    qsort(ops, num_operations, sizeof(Operation*),
        Operation::compare_operations_by_start_time);

    for (int i = 0; i < num_operations; i++) {
      ops[i]->deleted_ = false;
      ops[i]->id_ = i + 1;
      if (i == 0) {
        ops[i]->next_ = ops[i + 1];
        ops[i]->prev_ = head;
      } else if (i == num_operations - 1) {
        ops[i]->next_ = head;
        ops[i]->prev_ = ops[i - 1];
      } else {
        ops[i]->next_ = ops[i + 1];
        ops[i]->prev_ = ops[i - 1];
      }
    }
    head->next_ = ops[0];
    head->prev_ = ops[num_operations - 1];
  }

  Operation* begin() {
    return head_.next_;
  }
  void remove(Operation* op, int error) {
    op->remove();
    op->error_ = error;
    op->order_ = execution_id_++;
  }
  bool is_empty() const {
    return head_.next_ == &head_;
  }
  bool is_single_element() const {
    return !is_empty() && (head_.next_ == head_.prev_);
  }

  Operation** insert_ops(void) {
    return insert_ops_;
  }
  Operation** remove_ops(void) {
    return remove_ops_;
  }
  Operation** all_ops(void) {
    return ops_;
  }
  int num_all_ops(void) {
    return num_operations_;
  }
  int num_insert_ops(void) {
    return num_insert_ops_;
  }
  int num_remove_ops(void) {
    return num_remove_ops_;
  }

  void shorten_execution_times(int quotient);

  void CalculateOverlaps();

  class OverlapIterator {
  public:
    OverlapIterator(Operations* set) :
        set_(set), next_(set->head_.next_) {
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
    Iterator(Operation* head) :
        head_(head), next_(head_->next_) {
    }
    bool valid() const {
      return next_ != head_;
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
    const Operation* head_;
    Operation* next_;
  };

  Iterator elements() {
    return Iterator(&this->head_);
  }

  Iterator elements(Operation* head) {
    return Iterator(head);
  }

  OverlapIterator* Overlaps() {
    return new OverlapIterator(this);
  }

  static void test();
private:
  void insert(Operation* op);
  void match_operations(Operation** ops, int num_operations, bool adjust_start_times);

  // All operations as we read them.
  Operation** ops_;
  Operation** insert_ops_;
  Operation** remove_ops_;
  int num_operations_;
  int num_insert_ops_;
  int num_remove_ops_;
  // Operations sorted by invocation.
  Operation head_;
  int execution_id_;
};
#endif  // OPERATION_H
