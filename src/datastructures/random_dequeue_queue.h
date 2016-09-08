// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// Implementing the "Random Deqeued Queue" from:
//
// Y. Afek, G. Korland, and E. Yanovsky. quasi-linearizability: relaxed
// consistency for improved concurrency. In Proceedings of the 14th
// international conference on Principles of distributed systems, OPODIS’10,
// pages 395–410, Berlin, Heidelberg, 2010. Springer-Verlag.
//
// The more detailed tech report:
//
// Yehuda Afek, Guy Korland and Eitan Yanovsky,  Quasi-Linearizability: relaxed
// consistency for improved concurrency, Technical report, TAU '10.
//
// Available at (last accessed: 2013/01/08):
// https://docs.google.com/file/d/1dED19mzUmCozvl_PVufxux3vsWtUTTf3KFcanAyrH3io47nllSWGS9gTVait/edit?hl=en

#ifndef SCAL_DATASTRUCTURES_RANDOM_DEQUEUE_QUEUE_H_
#define SCAL_DATASTRUCTURES_RANDOM_DEQUEUE_QUEUE_H_

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "datastructures/queue.h"
#include "util/allocation.h"
#include "util/atomic_value_new.h"
#include "util/platform.h"
#include "util/random.h"

namespace scal {

namespace detail {

template<typename T>
struct Node : ThreadLocalMemory<kCachePrefetch>  {
  explicit Node(T item) : next(TaggedValue<Node<T>*>()),
                          value(item),
                          deleted(false) { }

  AtomicTaggedValue<Node<T>*, 64, 64> next;
  T value;
  bool deleted;
};

}  // namespace detail


template<typename T>
class RandomDequeueQueue : public Queue<T> {
 public:
  RandomDequeueQueue(uint64_t quasi_factor, uint64_t max_retries);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef detail::Node<T> Node;
  typedef TaggedValue<Node*> NodePtr;
  typedef AtomicTaggedValue<Node*, 64, 64> AtomicNodePtr;

  uint64_t quasi_factor_;
  uint64_t max_retries_;

  AtomicNodePtr* head_;
  AtomicNodePtr* tail_;
};

template<typename T>
RandomDequeueQueue<T>::RandomDequeueQueue(uint64_t quasi_factor,
                                          uint64_t max_retries)
    : quasi_factor_(quasi_factor),
      max_retries_(max_retries),
      head_(new AtomicNodePtr()),
      tail_(new AtomicNodePtr()) {
  Node* n = new Node((T)NULL);
  head_->store(NodePtr(n, 0));
  tail_->store(NodePtr(n, 0));
}


template<typename T> bool
RandomDequeueQueue<T>::enqueue(T item) {
  assert(item != (T)NULL);
  Node* n = new Node(item);
  NodePtr tail_old;
  NodePtr next;
  while (true) {
    tail_old = tail_->load();
    next = tail_old.value()->next.load();
    if (tail_old == tail_->load()) {
      if (next.value() == NULL) {
        NodePtr next_new(n, next.tag() + 1);
        if (tail_old.value()->next.swap(next, next_new)) {
          break;
        }
      } else {
        tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
      }
    }
  }
  tail_->swap(tail_old, NodePtr(n, tail_old.tag() + 1));
  return true;
}


template<typename T>
bool RandomDequeueQueue<T>::dequeue(T *item) {
  NodePtr tail_old;
  NodePtr head_old;
  NodePtr next;
  uint64_t retries = 0;
  uint64_t random_index;
  while (true) {
 TOP_WHILE:
    head_old = head_->load();
    tail_old = tail_->load();
    next = head_old.value()->next.load();
    if (head_->load() == head_old) {
      if (head_old == tail_old) {
        if (next.value() == NULL) {
          return false;
        }
        tail_->swap(tail_old, NodePtr(next.value(), tail_old.tag() + 1));
      } else {
        if (retries >= max_retries_) {
          random_index = 0;
        } else {
          random_index = pseudorand() % quasi_factor_;
        }
        retries++;

        Node* node = next.value();
        if (random_index == 0) {
          while (node != NULL && node->deleted == true) {
            NodePtr head_new(node, head_old.tag() + 1);
            if (!head_->swap(head_old, head_new) || node == tail_old.value()) {
              goto TOP_WHILE;
            }
            head_old = head_new;
            next = head_old.value()->next.load();
            node = next.value();
          }
          if (node == NULL) {
            return false;
          }
          if (node->deleted == false &&
              __sync_bool_compare_and_swap(&(node->deleted), false, true)) {
            *item = node->value;
            return true;
          }
        } else {
          uint64_t i;
          for (i = 0;
               (i < random_index) && (node->next.load().value() != NULL);
               ++i) {
            node = node->next.load().value();
          }
          if (node->deleted == false &&
              __sync_bool_compare_and_swap(&(node->deleted), false, true)) {
            *item = node->value;
            return true;
          }
        }
      }
    }
  }
  return true;
}

}  // namespace scal

#endif  // SCAL_DATASTRUCTURES_RANDOM_DEQUEUE_QUEUE_H_
