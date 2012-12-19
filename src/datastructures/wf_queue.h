// Copyright (c) 2012, the Scal project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Implementing the (non-optimized) wait-free queue from:
//
// A. Kogan and E. Petrank. Wait-free queues with multiple enqueuers and
// dequeuers. In Proceedings of the 16th ACM symposium on Principles and
// practice of parallel programming, PPoPP ’11, pages 223–234, New York, NY,
// USA, 2011. ACM.

#ifndef SCAL_DATASTRUCTURES_WF_QUEUE_H_
#define SCAL_DATASTRUCTURES_WF_QUEUE_H_

#include <assert.h>
#include <stdint.h> 

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/threadlocals.h"

namespace wf_details {

// Serves as container for a linked list element.
template<typename T>
struct Node {
  // Marks a node as free for other threads.
  static const uint64_t kTidNotSet = AtomicValue<uint64_t>::kValueMax-1;

  Node() {
    init((T)NULL, kTidNotSet);
  }

  Node(T data, uint64_t enq_tid) {
    init(data, enq_tid);
  }

  void init(T data, uint64_t enq_tid) {
    this->data = data;
    this->enq_tid = enq_tid;
    deq_tid.weak_set_value(kTidNotSet);
  }

  T data;
  AtomicPointer<Node<T>*> next;
  uint64_t enq_tid;
  AtomicValue<uint64_t> deq_tid;
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

}  // namespace wf_details

template<typename T>
class WaitfreeQueue {
 public:
  explicit WaitfreeQueue(uint64_t num_threads);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef wf_details::Node<T> Node;
  typedef wf_details::OperationDescriptor<T> OperationDescriptor;

  static const uint64_t kPtrAlignment = scal::kCachePrefetch;

  int64_t max_phase(void);
  inline bool is_still_pending(uint64_t thread_id, int64_t phase);
  void help(int64_t phase);
  void help_enqueue(uint64_t thread_id, int64_t phase);
  void help_dequeue(uint64_t thread_id, int64_t phase);
  void help_finish_enqueue(void);
  void help_finish_dequeue(void);

  uint64_t num_threads_;
  volatile AtomicPointer<Node*> *head_;
  volatile AtomicPointer<Node*> *tail_;
  volatile AtomicPointer<OperationDescriptor*> **state_;
};

template<typename T>
WaitfreeQueue<T>::WaitfreeQueue(uint64_t num_threads) {
  num_threads_ = num_threads;

  // Create sentinel node.
  Node *node = scal::get<Node>(kPtrAlignment);
  // Create aligned head and tail pointers, that point to the sentinel node.
  AtomicPointer<Node*> *head = scal::get<AtomicPointer<Node*>>(kPtrAlignment);
  head->weak_set_value(node);
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
}

template<typename T>
int64_t WaitfreeQueue<T>::max_phase(void) {
  int64_t max_phase = OperationDescriptor::kNoPhase;
  for (uint64_t i = 0; i < num_threads_; i++) {
    int64_t phase = state_[i]->value()->phase;
    if (phase > max_phase) {
      max_phase = phase;
    }
  }
  return max_phase;
}

template <typename T>
bool WaitfreeQueue<T>::is_still_pending(uint64_t thread_id, int64_t phase) {
  return state_[thread_id]->value()->pending &&
         state_[thread_id]->value()->phase <= phase;
}

template<typename T>
void WaitfreeQueue<T>::help(int64_t phase) {
  for (uint64_t i = 0; i < num_threads_; i++) {
    volatile OperationDescriptor *desc = state_[i]->value();
    if (desc->pending && desc->phase <= phase) {
      switch (desc->type) {
      case OperationDescriptor::Type::kEnqueue:
        help_enqueue(i, phase);
        break;
      case OperationDescriptor::Type::kDequeue:
        help_dequeue(i, phase);
        break;
      }
    }
  }
}

template<typename T>
bool WaitfreeQueue<T>::enqueue(T item) {
  assert(item != (T)NULL);
  int64_t phase = max_phase() + 1;
  uint64_t thread_id = threadlocals_get()->thread_id;
  Node *node = scal::tlget<Node>(kPtrAlignment);
  node->init(item, thread_id);
  OperationDescriptor *opdesc = scal::tlget<OperationDescriptor>(kPtrAlignment);
  opdesc->init(phase, true, OperationDescriptor::Type::kEnqueue, node);
  AtomicPointer<OperationDescriptor*> new_state(opdesc,
                                                state_[thread_id]->aba() + 1);
  state_[thread_id]->set_raw(new_state.raw());
  help(phase);
  help_finish_enqueue();
  return true;
}

template<typename T>
void WaitfreeQueue<T>::help_enqueue(uint64_t thread_id, int64_t phase) {
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
            help_finish_enqueue();
            return;
          }
        }
      } else {  // There's still an enqueue in progress.
        help_finish_enqueue();
      }
    }
  }
}

template<typename T>
void WaitfreeQueue<T>::help_finish_enqueue(void) {
  AtomicPointer<Node*> tail_old = *tail_;
  AtomicPointer<Node*> next = tail_old.value()->next;
  if (next.value() != NULL) {
    uint64_t thread_id = next.value()->enq_tid;
    AtomicPointer<OperationDescriptor*> cur_state = *state_[thread_id];
    if ((tail_old.raw() == tail_->raw())
        && ((state_[thread_id]->value())->node == next.value())) {
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
  }
}

template<typename T>
bool WaitfreeQueue<T>::dequeue(T *item) {
  int64_t phase = max_phase() + 1;
  uint64_t thread_id = threadlocals_get()->thread_id;
  OperationDescriptor *opdesc = scal::tlget<OperationDescriptor>(kPtrAlignment);
  opdesc->init(phase, true, OperationDescriptor::Type::kDequeue, NULL);
  AtomicPointer<OperationDescriptor*> new_state(opdesc,
                                                state_[thread_id]->aba() + 1);
  state_[thread_id]->set_raw(new_state.raw());
  help(phase);
  help_finish_dequeue();
  Node *node = state_[thread_id]->value()->node;
  if (node == NULL) {
    return false;
  }
  *item = node->next.value()->data;
  return true;
}

template<typename T>
void WaitfreeQueue<T>::help_dequeue(uint64_t thread_id, int64_t phase) {
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
          help_finish_enqueue();
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
        // We ignore ABA counter on this one.
        AtomicValue<uint64_t> old_deq_tid(Node::kTidNotSet, 0);
        AtomicValue<uint64_t> new_deq_tid(thread_id, 0);
        head_old.value()->deq_tid.cas(old_deq_tid, new_deq_tid);
        help_finish_dequeue();
      }
    }
  }
}

template<typename T>
void WaitfreeQueue<T>::help_finish_dequeue(void) {
  AtomicPointer<Node*> head_old = *head_;
  AtomicPointer<Node*> next = head_old.value()->next;
  uint64_t thread_id = head_old.value()->deq_tid.value();
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
      AtomicPointer<OperationDescriptor*> new_state(new_desc,
                                                    cur_state.aba() + 1);
      state_[thread_id]->cas(cur_state, new_state);
      AtomicPointer<Node*> head_new(next.value(), head_old.aba() + 1);
      head_->cas(head_old, head_new);
    }
  }
}

#endif  // SCAL_DATASTRUCTURES_WF_QUEUE_H_
