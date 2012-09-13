#include <stdio.h>
#include <assert.h>
#include <list>
#include <string.h>

#include "operation.h"
#include "trace.h"

Trace::Entry::Entry(int id, int flag) : id_(id), flag_(flag) {
}

Trace::Trace(int length) : length_(length), size_(0) {
  operations_ = new Entry[length];
}

Trace::Trace(const Trace& other) : length_(other.length_), size_(other.size_) {
  operations_ = new Entry[length_];
  memcpy(operations_, other.operations_, sizeof(Entry) * size_);
}

Trace::~Trace() {
  delete [] operations_;
}

void Trace::ensureSize(int length) {
  if (length_ < length) {
    int new_len = length_;
    while (new_len <= length){
      new_len *= 2;
    }
    Entry* new_ops = new Entry[new_len];

    memcpy(new_ops, operations_, sizeof(Entry) * size());
    delete [] operations_;

    operations_ = new_ops;
    length_ = new_len;
  }
}


void Trace::append(Operation* op) {
  Entry entry(op->id(), 0);
  ensureSize(size_ + 1);
  operations_[size_++] = entry;
}

void Trace::remove(Operation* rm_op) {
  if (size() > 0) {
    Entry entry = operations_[--size_]; 
    assert(entry.id() == rm_op->id());
  }
}


void Trace::print() const {
  printf("<");
  for (size_t i = 0; i < size() - 1; ++i)
    printf("(%d, %x), ", operations_[i].id(), operations_[i].flag());

  printf("(%d,%d)>",
         operations_[size() -1].id(),
         operations_[size() -1].flag());
}

void Trace::test() {

}


void Traces::merge(Traces* traces) {
  traces_.splice(traces_.end(), traces->traces_);
}

void Traces::append(const Trace& trace) {
  traces_.push_back(trace);
}

void Traces::appendAll(Operation* op) {
  if (traces_.size() == 0) {
    Trace trace(32);
    trace.append(op);
    traces_.push_back(trace);
  } else {
    for (list<Trace>::iterator it=traces_.begin(); it != traces_.end(); it++ ) {
      it->append(op);
    }
  }
}

void Traces::removeAll(Operation* op) {
  for (list<Trace>::iterator it=traces_.begin(); it != traces_.end(); it++ ) {
    it->remove(op);
  }
}


void Traces::print() const {
  for (list<Trace>::const_iterator it=traces_.begin(); it != traces_.end(); it++ ) {
    it->print();
    printf("\n");
  }
}


void Traces::test() {

}

