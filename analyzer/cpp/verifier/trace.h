#ifndef TRACE_H
#define TRACE_H

#include "operation.h"
#include <list>

using std::list;

class Trace {
  public:
    class Entry {
      public:
        Entry() : id_(0), flag_(0) {}
        Entry(int id, int flag);
        int id() const { return id_; };
        int flag() const { return flag_; }
        void set_flag(int flag) { flag_ = flag; }
      private:
        uint32_t id_;
        uint32_t flag_;
    };

    Trace(int size);
    Trace(const Trace& other);
    ~Trace();
    void append(Operation* op);
    void remove(Operation* op);
    void print() const;
    int id(int i) const { return operations_[i].id(); }
    void set_flag (int i, int flag) { operations_[i].set_flag(flag); }
    int flag(int i) const { return operations_[i].flag(); }
    size_t size() const { return size_; }
    static void test();
  private:
    void ensureSize(int length);
    Entry* operations_;
    int length_;
    int size_;
};


class Traces {
  public:
    typedef list<Trace>::iterator Iterator;
    int size() const { return traces_.size(); }
    void print() const;
    void merge(Traces* traces);
    void append(const Trace& trace);
    void appendAll(Operation* op);
    void removeAll(Operation* op);

    void operator=(const Traces& traces) { traces_ = traces.traces_; }
    Iterator begin() { return traces_.begin(); }
    Iterator end() { return traces_.end(); }
    static void test();
  private:
    list<Trace> traces_;
};

#endif  // TRACE_H
