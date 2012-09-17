#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <list>
#include <algorithm>
#include <string.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "histogram.h"
#include "fifoExecuterLowerBound.h"

Operation::Operation() :
    start_(0), real_start_(0), end_(0), real_end_(0), type_(Operation::INSERT), value_(0),
    next_(this), prev_(this), deleted_(false), error_(0), id_(0), order_(0), matching_op_(NULL){

//  Time start_;
//      Time real_start_;
//      Time real_end_;
//      Time end_;
//      int id_;
//      OperationType type_;
//      int value_;
//      Operation* next_;
//      Operation* prev_;
//      vector<int> overlaps_;
//      vector<int> insert_overlaps_value_;
//      vector<int> remove_overlaps_value_;
//      bool deleted_;
//      int order_;
//      int lin_order_;
//      int error_;
//      Operation* matching_op_;
//      friend class Operations;
}

Operation::Operation(Time start, Time end) :
    start_(start), real_start_(start), end_(end), real_end_(end), type_(Operation::INSERT), value_(0),
    next_(this), prev_(this), deleted_(false), error_(0), id_(0), order_(0), matching_op_(NULL) {
}

Operation::Operation(Time start, Time end, int id) :
    start_(start), real_start_(start), end_(end), real_end_(end), type_(Operation::INSERT), value_(0),
    next_(this), prev_(this), deleted_(false), error_(0), id_(id), order_(0), matching_op_(NULL) {
}

Operation::Operation(Time start, Time end, OperationType type, int value) :
    start_(start), real_start_(start), end_(end), real_end_(end), type_(type), value_(value),
    next_(this), prev_(this), deleted_(false), error_(0), id_(0), order_(0), matching_op_(NULL) {

}

bool Operation::operator<(const Operation& op) const {
  if (start_ < op.start_) {
    return true;
  }

  if (start_ == op.start_) {
    return end_ < op.end_;
  }

  return false;
}

bool Operation::operator<=(const Operation& op) const {
  if (start_ < op.start_) {
    return true;
  }

  if (start_ == op.start_) {
    return end_ <= op.end_;
  }

  return false;
}

bool Operation::operator>(const Operation& op) const {
  if (start_ > op.start_)
    return true;

  if (start_ == op.start_)
    return end_ > op.end_;

  return false;
}

bool Operation::operator>=(const Operation& op) const {
  if (start_ > op.start_)
    return true;

  if (start_ == op.start_)
    return end_ >= op.end_;

  return false;
}

void Operation::test() {
  Operation a(0, 5);
  Operation b(2, 6);

  assert(a < b);
  assert(b > a);
}

void Operation::print() const {
  printf("%16" PRId64 " %16" PRId64 " %6d %6d %6d %6d %6d overlaps: ", real_start_,
      end_, id_, type_, value_, order_, error_);
//  for (size_t i = 0; i < overlaps_.size(); i++) {
//    printf("%d ", overlaps_[i]);
//  }
  printf("\n");
}



/**
 * Compares the start times of remove operations with the start times
 * of the matching insert operations. If the matching insert operation
 * starts later than the remove operation the start time of the remove
 * operation is set to the start time of the insert operation because
 * the remove operation cannot take effect before the insert operation
 * has even started.
 */
void Operations::adjust_start_times(Operation** ops, int num_operations) {
  insert_ops_ = new Operation*[num_operations];
  remove_ops_ = new Operation*[num_operations];
  int insert_index = 0;
  int remove_index = 0;
  for (int i = 0; i < num_operations; i++) {
    if (ops[i]->type_ == Operation::INSERT) {
      insert_ops_[insert_index] = ops[i];
      insert_index++;
    } else {
      remove_ops_[remove_index] = ops[i];
      remove_index++;
    }
  }

  num_insert_ops_ = insert_index;
  num_remove_ops_ = remove_index;

  qsort(insert_ops_, num_insert_ops_, sizeof(Operation*), Operation::compare_ops_by_value);
  qsort(remove_ops_, num_remove_ops_, sizeof(Operation*), Operation::compare_ops_by_value);
  int j = 0;
  for (int i = 0; i < num_remove_ops_; i++) {

    int value = remove_ops_[i]->value_;
    if (value != -1) {
      while (insert_ops_[j]->value_ < value) {
        j++;
        if (j >= num_insert_ops_) {
          fprintf(stderr,
              "FATAL3: The value %d gets removed but never inserted.\n", value);
          exit(-1);
        }
        assert(j < num_insert_ops_);
      }

      if (insert_ops_[j]->value_ != value) {
        fprintf(stderr,
            "FATAL4: The value %d gets removed but never inserted.\n", value);
        exit(-1);
      }

      insert_ops_[j]->matching_op_ = remove_ops_[i];
      remove_ops_[i]->matching_op_ = insert_ops_[j];

      // If the insert operation starts before the remove operation let them
      // both start at the same time, the remove operation cannot take effect
      // before the remove operation has started.
      if (insert_ops_[j]->start_ > remove_ops_[i]->start_) {
        if (insert_ops_[j]->start_ > remove_ops_[i]->end_) {
          fprintf(stderr,
              "FATAL5: The insert operation of value %d starts after its remove operation has ended.\n",
              value);
          exit(-1);
        }
        remove_ops_[i]->start_ = insert_ops_[j]->start_;
      }

      // If the insert operation ends after the remove operation let them both
      // end at the same time, because the insert operation cannot take effect
      // after the element has already been removed again.
      if(remove_ops_[i]->end_ < insert_ops_[j]->end_) {
        insert_ops_[j]->end_ = remove_ops_[i]->end_;
      }
    }
  }
}



void Operations::Initialize(Operation** ops, int num_operations) {
  ops_ = ops;
  num_operations_ = num_operations;

  // Adjust the start times of remove operations which start before their matching insert operations.
  adjust_start_times(ops, num_operations);
  // No need to adjust the end times of insert operations, the adjustment of the start times of
  // remove operations fixes the problem.

  create_doubly_linked_list(&head_, ops_, num_operations_);
}

Operations::Operations(FILE* input, int num_ops) :
    num_operations_(num_ops), head_(0, 0, 0) {
  head_.deleted_ = false;
  Operation** ops = new Operation*[num_operations_];
  for (int i = 0; i < num_operations_; ++i) {
//    int type;
    Time op_start;
    int thread_id;
    Time op_end;
    char type[8];
    Operation::OperationType op_type;
    int op_value;
    if (fscanf(input, "profile: %s %d %"PRIu64" %"PRIu64" %d\n", type, &thread_id, &op_start, &op_end, &op_value) == EOF) {
      fprintf(stderr, "ERROR: could not read all %d elements, abort after %d\n",
          num_operations_, i);
      exit(2);
    }
    if (strcmp(type, "ds_put") == 0) {
      op_type = Operation::INSERT;
    } else {
      op_type = Operation::REMOVE;
    }
    ops[i] = new Operation(op_start, op_end, op_type, op_value);
  }

  printf("Finished reading file \n");


  Initialize(ops, num_ops);
  printf("Finished Initialize \n");
}

bool Operations::OverlapIterator::has_next() const {
  if (next_ == &set_->head_) {
    return false;
  }

  if (next_->start() >= from_ && next_->start() <= until_) {
    return true;
  }

  return false;
}

//const Operation** Operations::operator[](int i) const {
//  return &ops_[i - 1];
//}

Operation** Operations::operator[](int i) {
  return &ops_[i - 1];
}

Operation* Operations::OverlapIterator::get_next() {
  Operation* ret = next_;
  next_ = ret->next_;
  if (next_->end_ < until_)
    until_ = next_->end_;

  return ret;
}

bool Operations::is_consistent() {
  Operation* iter = head_.next_;
  while (iter != &head_) {
    if (iter->next_ == &head_)
      break;
    if (*iter > *iter->next_)
      return false;
    iter = iter->next_;
  }

  return true;
}

void Operations::test() {
  Operations ops;
  assert(ops.is_empty());

  Operation a(0, 5, 1);
  Operation b(2, 3, 2);
  Operation c(3, 5, 3);
  Operation d(4, 5, 4);
  Operation e(6, 7, 5);

  ops.insert(&e);
  ops.insert(&a);
  ops.insert(&d);
  ops.insert(&c);
  ops.insert(&b);

  assert(!ops.is_empty());

  ops.print();
  assert(ops.is_consistent());

  OverlapIterator* overlaps = ops.Overlaps();
  {
    int result[] = { 1, 2, 3 };
    int i = 0;
    while (overlaps->has_next()) {
      Operation* next = overlaps->get_next();
      assert(next->id() == result[i++]);
      next->print();
    }
    printf("\n");
  }
  delete overlaps;

  overlaps = ops.Overlaps();
  while (overlaps->has_next()) {
    Operation* next = overlaps->get_next();
    next->remove();
    ops.print();
    next->reinsert();
  }

  {
    Iterator iter = ops.elements();
    int result[] = { 1, 2, 3, 4, 5 };
    int i = 0;
    for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {
      assert(element->id() == result[i++]);
      element->print();
    }
    assert(i == 5);
  }
}

void Operations::print() {
  Operation* iter = head_.next_;
  while (iter != &head_) {
    iter->print();
    iter = iter->next_;
  }
}

// insert op just before this element.
void Operation::insert_before(Operation* op) {
  deleted_ = false;
  prev_ = op->prev_;
  next_ = op;
  prev_->next_ = this;
  op->prev_ = this;
}

void Operations::insert(Operation* new_op) {
  Operation* iter = head_.prev_;

  // find first element with start time later than new_op.
  while (iter != &head_) {
    if (*iter <= *new_op)
      break;
    iter = iter->prev_;
  }
  iter = iter->next_;

  new_op->insert_before(iter);
}

void Operations::CalculateOverlaps() {
  Iterator iter = elements();
  Time last_start_time = 0;
  for (Operation* element = iter.get(); iter.valid(); element = iter.step()) {
    assert(element->start() >= last_start_time);
    last_start_time = element->start();
    Operation* tmp = element->next_;
    while (tmp != &head_ && element->end() >= tmp->start()) {
      assert(!tmp->deleted());

      element->overlaps_.push_back(tmp->id());
      if (element->type() == Operation::INSERT && tmp->type() == Operation::INSERT) {
        element->overlaps_insert_ops_.push_back(tmp);
//        element->insert_overlaps_value_.push_back(tmp->value());
      }
//      else {
//        element->remove_overlaps_value_.push_back(tmp->value());
//      }
      tmp = tmp->next_;
    }
//    sort(element->overlaps_.begin(), element->overlaps_.end());
//    sort(element->insert_overlaps_value_.begin(),
//        element->insert_overlaps_value_.end());
//    sort(element->remove_overlaps_value_.begin(),
//        element->remove_overlaps_value_.end());
  }
}

static bool compOpLowerBound(const ErrorEntry& a, const ErrorEntry& b) {

  if (a.error_ < b.error_) {
    return true;
  }
  if (a.error_ == b.error_) {

    // TODO: Fix the assignment of the id's. At the moment the id is assigned before the sorting
    // and does not represent the order of the invocation time.
    return a.op_->id() + a.matching_insert_->id()
        < b.op_->id() + b.matching_insert_->id();
  }
  return false;
}

/**
 * Let op be the current operation, let overlaps(op) be the set of all operations which overlap with op but start
 * later than op. The set firstOverlaps(op) is the set of all remove operations o in overlaps(op) such that there does
 * not exist another remove operation o' in overlaps(op) such that o.start < o'.end. This function calculates the number
 * of remove operations o which are in firstOverlaps(op) and whose corresponding insert operation ins(o)
 *  is strictly before the insert operation ins(op) corresponding to op, i.e. ins(o).start < ins(op).start and there
 *  exists an insert operation ins(o') which starts before ins(o) and overlaps with ins(o) but not with ins(op).
 *
 *  Null returns are ignored for the calculation of the firstOverlaps set.
 */
void Operation::determineExecutionOrderLowerBound(Operations& ops,
    FifoExecuter* executer, int errorDistance,
    vector<ErrorEntry>* result,
    Time* linearizable_time_window_end) {


  sort(overlaps_.begin(), overlaps_.end());

  *linearizable_time_window_end = end();

  for (size_t i = 0; i < overlaps_.size(); i++) {
    Operation* element = *ops[overlaps_[i]];
    // AH: < or <=?
    if (element->start() <= *linearizable_time_window_end) {
      // AH: What is element->delete? It is not mentioned in the documentation of the function.
      if (!element->deleted() && element->value() != -1
          && element->type() == REMOVE) {
        Operation* matchingInsert;
        int errorDistanceTmp = executer->semanticalError(element,
            &matchingInsert);
        if (errorDistanceTmp < errorDistance) {
          ErrorEntry e(element, errorDistanceTmp, matchingInsert);
          result->push_back(e);
        }
        if (element->end() < *linearizable_time_window_end) {
          *linearizable_time_window_end = element->end();
        }
      }
    } else {
      break;
    }
  }
  sort(result->begin(), result->end(), compOpLowerBound);
}

static bool compOpUpperBound(const ErrorEntry& a, const ErrorEntry& b) {
  return a.error_ > b.error_;
}

void Operation::determineExecutionOrderUpperBound(Operations& ops,
    FifoExecuter* executer, int errorDistance,
    vector<ErrorEntry>* result) const {
  Time linearizable_time_window_end = end();
  for (size_t i = 0; i < overlaps_.size(); i++) {
    Operation* element = *ops[overlaps_[i]];
    if (element->start() < linearizable_time_window_end) {
      if (!element->deleted() && element->type() == REMOVE) {
        int errorDistanceTmp = 0;
        Operation *matchingInsert = NULL;
        if (element->value() != -1) {
          errorDistanceTmp = executer->semanticalError(element,
              &matchingInsert);
        } else {
          errorDistanceTmp = executer->amountOfStartedEnqueueOperations(
              element);
        }
        if (errorDistanceTmp > errorDistance) {
          ErrorEntry e(element, errorDistanceTmp, matchingInsert);
          result->push_back(e);
        }
        if (element->end() < linearizable_time_window_end) {
          linearizable_time_window_end = element->end();
        }
      }
    } else {
      break;
    }
  }
  sort(result->begin(), result->end(), compOpUpperBound);
}

