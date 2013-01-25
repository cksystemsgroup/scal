// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the wait-free queue from:
//
// A. Kogan and E. Petrank. A methodology for creating fast wait-free data
// structures. In Proceedings of the 17th ACM SIGPLAN symposium on Principles
// and Practice of Parallel Programming, PPoPP ’12, pages 141–150, New York, NY,
// USA, 2012. ACM.

// Note: This implementation may suffer from the ABA problem, as the head
// pointer's reference counter (head_->aba()) is used by the algorithm to stamp
// the dequeueing thread.  The problem could be solved by using the approach of
// their PPoPP'11 paper, where deq_tid is stored in the node.

#ifndef SCAL_DATASTRUCTURES_WF_QUEUE_PPOPP12_H_
#define SCAL_DATASTRUCTURES_WF_QUEUE_PPOPP12_H_

#include <assert.h>
#include <stdint.h> 

#include "datastructures/queue.h"
#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace wf_ppopp12_details {

// Serves as container for a linked list element.
template<typename T>
struct Node {
  // Marks a node as free for other threads.
  static const uint64_t kTidNotSet = AtomicValue<uint64_t>::kValueMax - 1;

  Node() {
    init((T)NULL);
  }

  explicit Node(T data) {
    init(data);
  }

  void init(T data) {
    this->data = data;
    this->enq_tid = kTidNotSet;
  }

  T data;
  AtomicPointer<Node<T>*> next;
  uint64_t enq_tid;
};

// Descriptor describes the state of each thread's operation.
template<typename T>
struct OperationDescriptor {
  enum Type {
    kEnqueue,
    kDequeue
  };

  static const int64_t kNoPhase = -1;

  OperationDescriptor(void) {}

  OperationDescriptor(int64_t phase, bool pending, Type type, Node<T> *node) {
    init(phase, pending, type, node);
  }

  void init(int64_t phase, bool pending, Type type, Node<T>* node) {
    this->phase = phase;
    this->pending = pending;
    this->type = type;
    this->node = node;
  }

  int64_t phase;
  bool pending;
  Type type;
  Node<T> *node;
};

template<typename T>
class HelpRecord {
 public:
  static void prepare(uint64_t num_threads,
                      uint64_t helping_delay,
                      volatile AtomicPointer<OperationDescriptor<T>*> **state) {
    NumThreads = num_threads;
    HelpingDelay = helping_delay;
    State = state;
  }

  HelpRecord() {
    cur_thread_id_ = -1;
    reset();
  }

  void reset(void) volatile {
    cur_thread_id_ = (cur_thread_id_ + 1) % NumThreads;
    last_phase_ = State[cur_thread_id_]->value()->phase;
    next_check_ = HelpingDelay;
  }

  bool do_next_check(void) volatile {
    if (next_check_-- == 0) {
      return true;
    }
    return false;
  }

  uint64_t cur_thread_id() volatile {
    return cur_thread_id_;
  } 

  uint64_t last_phase() volatile {
    return last_phase_;
  }

 private:
  static uint64_t NumThreads;
  static uint64_t HelpingDelay;
  static volatile AtomicPointer<OperationDescriptor<T>*> **State;

  int64_t cur_thread_id_;
  uint64_t last_phase_;
  uint64_t next_check_;
};

template<typename T>
uint64_t HelpRecord<T>::NumThreads = 0;

template<typename T>
uint64_t HelpRecord<T>::HelpingDelay = 0;

template<typename T>
volatile AtomicPointer<OperationDescriptor<T>*>** HelpRecord<T>::State = NULL;


}  // namespace wf_ppopp12_details

template<typename T>
class WaitfreeQueue : public Queue<T> {
 public:
  WaitfreeQueue(uint64_t num_threads,
                uint64_t max_retries,
                uint64_t helping_delay);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef wf_ppopp12_details::Node<T> Node;
  typedef wf_ppopp12_details::OperationDescriptor<T> OperationDescriptor;
  typedef wf_ppopp12_details::HelpRecord<T> HelpRecord;

  static const uint64_t kPtrAlignment = scal::kCachePrefetch;

  void fix_tail(AtomicPointer<Node*> tail_old, AtomicPointer<Node*> next);
  void wf_enq(Node *node);
  bool wf_deq(T *item);
  void help_enq(uint64_t thread_id, int64_t phase);
  void help_deq(uint64_t thread_id, int64_t phase);
  void help_finish_enq();
  void help_finish_deq();
  void help_if_needed();
  inline bool is_still_pending(uint64_t thread_id, int64_t phase);

  uint64_t num_threads_;
  uint64_t max_retries_;
  uint64_t helping_delay_;
  volatile AtomicPointer<Node*> *head_;
  volatile AtomicPointer<Node*> *tail_;
  volatile AtomicPointer<OperationDescriptor*> **state_;
  volatile HelpRecord **records_;
};

template<typename T>
WaitfreeQueue<T>::WaitfreeQueue(uint64_t num_threads,
                                uint64_t max_retries,
                                uint64_t helping_delay) {
  num_threads_ = num_threads;
  max_retries_ = max_retries;
  helping_delay_ = helping_delay;

  // Create sentinel node.
  Node *node = scal::get<Node>(kPtrAlignment);
  // Create aligned head and tail pointers, that point to the sentinel node.
  AtomicPointer<Node*> *head = scal::get<AtomicPointer<Node*>>(kPtrAlignment);
  head->weak_set_value(node);
  head->set_aba(Node::kTidNotSet);
  head_ = const_cast<volatile AtomicPointer<Node*>*>(head);
  AtomicPointer<Node*> *tail = scal::get<AtomicPointer<Node*>>(kPtrAlignment);
  tail->weak_set_value(node);
  tail_ = const_cast<volatile AtomicPointer<Node*>*>(tail);

  // Each thread gets its own OperationDescriptor.
  state_ = const_cast<volatile AtomicPointer<OperationDescriptor*>**>(
      static_cast<AtomicPointer<OperationDescriptor*>**>(calloc(
          num_threads_, sizeof(AtomicPointer<OperationDescriptor*>*))));
  for (uint64_t i = 0; i < num_threads_; i++) {
    OperationDescriptor *opdesc = scal::get<OperationDescriptor>(kPtrAlignment);
    opdesc->init(OperationDescriptor::kNoPhase,
                 false,
                 OperationDescriptor::Type::kEnqueue,
                 NULL);
    state_[i] = scal::get<AtomicPointer<OperationDescriptor*>>(kPtrAlignment);
    state_[i]->weak_set_value(opdesc);
  }

  HelpRecord::prepare(num_threads_, helping_delay_, state_);
  // Each thread gets its own HelpRecord.
  records_ = const_cast<volatile HelpRecord**>(static_cast<HelpRecord**>(calloc(
      num_threads_, sizeof(*records_))));
  for (uint64_t i = 0; i < num_threads_; i++) {
    records_[i] = scal::get<HelpRecord>(kPtrAlignment);
  }
}

template<typename T>
void WaitfreeQueue<T>::help_if_needed() {
  uint64_t thread_id = threadlocals_get()->thread_id;
  volatile HelpRecord *rec = records_[thread_id];
  if (rec->do_next_check()) {
    OperationDescriptor *desc = state_[rec->cur_thread_id()]->value();
    if (desc->pending && desc->phase == (int64_t)rec->last_phase()) {
      if (desc->type == OperationDescriptor::Type::kEnqueue) {
        help_enq(rec->cur_thread_id(), rec->last_phase());
      } else {
        help_deq(rec->cur_thread_id(), rec->last_phase());
      }
    }
  }
}

template<typename T>
void WaitfreeQueue<T>::fix_tail(AtomicPointer<Node*> tail_old,
                                AtomicPointer<Node*> next) {
  if (next.value()->enq_tid == Node::kTidNotSet) {  // fast path enqueue
    AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
    tail_->cas(tail_old, tail_new);
  } else {  // slow path enqueue
    help_finish_enq();
  }
}

template <typename T>
bool WaitfreeQueue<T>::is_still_pending(uint64_t thread_id, int64_t phase) {
  return state_[thread_id]->value()->pending &&
         state_[thread_id]->value()->phase <= phase;
}

template<typename T>
bool WaitfreeQueue<T>::enqueue(T item) {
  assert(item != (T)NULL);
  help_if_needed();
  Node *node = scal::tlget<Node>(kPtrAlignment);
  node->init(item);
  unsigned int trials = 0;
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> next;
  while (trials++ < max_retries_) {
    tail_old = *tail_;
    next = tail_old.value()->next;
    if (tail_old.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        AtomicPointer<Node*> next_new(node, next.aba() + 1);
        if (tail_old.value()->next.cas(next, next_new)) {
          return true;
        }
      } else {
        fix_tail(tail_old, next);
      }
    }
  }
  wf_enq(node);
  return true;
}

template<typename T>
bool WaitfreeQueue<T>::dequeue(T *item) {
  help_if_needed();
  unsigned int trials = 0;
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> next;
  while (trials++ < max_retries_) {
    head_old = *head_;
    tail_old = *tail_;
    next = head_old.value()->next;
    if (head_->raw() == head_old.raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          return false;
        }
        fix_tail(tail_old, next);
      } else if (head_->aba() == Node::kTidNotSet) {  // no slow deq
        *item = next.value()->data;
        AtomicPointer<Node*> head_new(next.value(), Node::kTidNotSet);
        if (head_->cas(head_old, head_new)) {
          return true;
        }
      } else {  // slow deq
        help_finish_deq();
      }
    }
  }
  return wf_deq(item);
}

template<typename T>
void WaitfreeQueue<T>::wf_enq(Node *node) {
  uint64_t thread_id = threadlocals_get()->thread_id;
  int64_t phase = state_[thread_id]->value()->phase + 1;
  node->enq_tid = thread_id;
  OperationDescriptor *opdesc = scal::tlget<OperationDescriptor>(kPtrAlignment);
  opdesc->init(phase, true, OperationDescriptor::Type::kEnqueue, node);
  AtomicPointer<OperationDescriptor*> state_new(
      opdesc, state_[thread_id]->aba() + 1);
  state_[thread_id]->set_raw(state_new.raw());
  help_enq(thread_id, phase);
  help_finish_enq();
}

template<typename T>
void WaitfreeQueue<T>::help_enq(uint64_t thread_id, int64_t phase) {
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> next;
  while (is_still_pending(thread_id, phase)) {
    tail_old = *tail_;
    next = tail_old.value()->next;
    if (tail_old.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        if (is_still_pending(thread_id, phase)) {
          AtomicPointer<Node*> new_next(state_[thread_id]->value()->node,
                                        next.aba() + 1);
          if (tail_old.value()->next.cas(next, new_next)) {
            help_finish_enq();
            return;
          }
        }
      } else {  // There's still an enqueue in progress.
        help_finish_enq();
      }
    }
  }
}

template<typename T>
void WaitfreeQueue<T>::help_finish_enq() {
  AtomicPointer<Node*> tail_old = *tail_;
  AtomicPointer<Node*> next = tail_old.value()->next;
  if (next.value() != NULL) {
    uint64_t thread_id = next.value()->enq_tid;
    if (thread_id != Node::kTidNotSet) {
      AtomicPointer<OperationDescriptor*> cur_state = *state_[thread_id];
      if ((tail_old.raw() == tail_->raw()) &&
          ((state_[thread_id]->value())->node == next.value())) {
        OperationDescriptor *new_desc =
            scal::tlget<OperationDescriptor>(kPtrAlignment);
        new_desc->init(state_[thread_id]->value()->phase, false,
                       OperationDescriptor::Type::kEnqueue, next.value());
        AtomicPointer<OperationDescriptor*> new_state(new_desc,
                                                      cur_state.aba() + 1);
        state_[thread_id]->cas(cur_state, new_state);
        AtomicPointer<Node*> new_tail(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, new_tail);
      } 
    } else {  // node was appended on the fast path
      AtomicPointer<Node*> new_tail(next.value(), tail_old.aba() + 1);
      tail_->cas(tail_old, new_tail);
    }
  }
}

template<typename T>
bool WaitfreeQueue<T>::wf_deq(T *item) {
  uint64_t thread_id = threadlocals_get()->thread_id;
  int64_t phase = state_[thread_id]->value()->phase + 1;
  OperationDescriptor *opdesc = scal::tlget<OperationDescriptor>(kPtrAlignment);
  opdesc->init(phase, true, OperationDescriptor::Type::kDequeue, NULL);
  AtomicPointer<OperationDescriptor*> state_new(
      opdesc, state_[thread_id]->aba() + 1);
  state_[thread_id]->set_raw(state_new.raw());
  help_deq(thread_id, phase);
  help_finish_deq();
  Node *node = state_[thread_id]->value()->node;
  if (node == NULL) {
    return false;
  }
  *item = node->next.value()->data;
  return true;
}

template<typename T>
void WaitfreeQueue<T>::help_deq(uint64_t thread_id, int64_t phase) {
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> next;
  while (is_still_pending(thread_id, phase)) {
    head_old = *head_;
    tail_old = *tail_;
    next = head_old.value()->next;
    if (head_->raw() == head_old.raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {  // Queue is empty.
          AtomicPointer<OperationDescriptor*> cur_state = *state_[thread_id];
          if (tail_old.value() == tail_->value()
              && is_still_pending(thread_id, phase)) {
            OperationDescriptor *new_desc =
                scal::tlget<OperationDescriptor>(kPtrAlignment);
            new_desc->init(state_[thread_id]->value()->phase,
                           false,
                           OperationDescriptor::Type::kDequeue,
                           NULL);
            AtomicPointer<OperationDescriptor*> new_state(new_desc,
                                                        cur_state.aba() + 1);
            // If the next CAS fails, another thread changed the state, which
            // is also ok since the descriptor will not indicate pending in the
            // next try.
            state_[thread_id]->cas(cur_state, new_state);
          }
        } else {  // Help finish a pending enqueue.
          help_finish_enq();
        }
      } else {  // Queue is not empty.
        AtomicPointer<OperationDescriptor*> cur_state = *state_[thread_id];
        OperationDescriptor *cur_desc = cur_state.value();
        Node *node = cur_desc->node;
        if (!is_still_pending(thread_id, phase)) {
          break;
        }
        if (head_->raw() == head_old.raw()
            && node != head_old.value()) {
          OperationDescriptor *new_desc =
              scal::tlget<OperationDescriptor>(kPtrAlignment);
          new_desc->init(state_[thread_id]->value()->phase, true,
                         OperationDescriptor::Type::kDequeue,
                         head_old.value());
          AtomicPointer<OperationDescriptor*> new_state(new_desc,
              cur_state.aba() + 1);
          if (!state_[thread_id]->cas(cur_state, new_state)) {
            continue;
          }
        }
        AtomicPointer<Node*> head_expected(head_old.value(), Node::kTidNotSet);
        AtomicPointer<Node*> head_new(head_old.value(), thread_id);
        head_->cas(head_expected, head_new);
        help_finish_deq();
      }
    }
  }
}

template<typename T>
void WaitfreeQueue<T>::help_finish_deq(void) {
  AtomicPointer<Node*> head_old = *head_;
  AtomicPointer<Node*> next = head_old.value()->next;
  uint64_t thread_id = head_old.aba();
  if (thread_id != Node::kTidNotSet) {
    AtomicPointer<OperationDescriptor*> cur_state = *state_[thread_id];
    if (head_old.raw() == head_->raw()
        && next.value() != NULL) {
      OperationDescriptor *new_desc =
          scal::tlget<OperationDescriptor>(kPtrAlignment);
      new_desc->init(state_[thread_id]->value()->phase,
                     false,
                     OperationDescriptor::Type::kDequeue,
                     state_[thread_id]->value()->node);
      AtomicPointer<OperationDescriptor*> state_new(new_desc,
                                                    cur_state.aba() + 1);
      state_[thread_id]->cas(cur_state, state_new);
      AtomicPointer<Node*> head_new(next.value(), Node::kTidNotSet);
      head_->cas(head_old, head_new);
    }
  }
}

#endif  // SCAL_DATASTRUCTURES_WF_QUEUE_PPOPP12_H_
