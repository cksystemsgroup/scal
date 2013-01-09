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
#include <stdint.h>
#include <stdio.h>

#include "util/atomic_value.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

namespace rd_details {

template<typename T>
struct Node {
  AtomicPointer<Node<T>*> next;
  T value;
  bool deleted;
};

}  // namespace rd_details

template<typename T>
class RandomDequeueQueue {
 public:
  RandomDequeueQueue(uint64_t quasi_factor, uint64_t max_retries);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef rd_details::Node<T> Node;

  uint64_t quasi_factor_;
  uint64_t max_retries_;
  AtomicPointer<Node*> *head_;
  AtomicPointer<Node*> *tail_;
};

template<typename T>
RandomDequeueQueue<T>::RandomDequeueQueue(uint64_t quasi_factor,
                                          uint64_t max_retries) {
  quasi_factor_ = quasi_factor;
  Node *n = scal::get<Node>(scal::kCachePrefetch);
  head_ = scal::get<AtomicPointer<Node*>>(scal::kPageSize);
  tail_ = scal::get<AtomicPointer<Node*>>(scal::kPageSize);
  head_->set_value(n);
  tail_->set_value(n);
}

template<typename T> bool
RandomDequeueQueue<T>::enqueue(T item) {
  assert(item != (T)NULL);
  Node *node = scal::tlget<Node>(scal::kCachePrefetch);
  node->value = item;
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> next;
  while (true) {
    tail_old = *tail_;
    next = tail_old.value()->next;
    if (tail_old.raw() == tail_->raw()) {
      if (next.value() == NULL) {
        AtomicPointer<Node*> next_new(node, next.aba() + 1);
        if (tail_old.value()->next.cas(next, next_new)) {
          break;
        }
      } else {
        AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
      }
    }
  }
  AtomicPointer<Node*> tail_new(node, tail_old.aba() + 1);
  tail_->cas(tail_old, tail_new);
  return true;
}

template<typename T>
bool RandomDequeueQueue<T>::dequeue(T *item) {
  AtomicPointer<Node*> tail_old;
  AtomicPointer<Node*> head_old;
  AtomicPointer<Node*> next;
  uint64_t retries = 0;
  uint64_t random_index;
  while (true) {
 TOP_WHILE:
    head_old = *head_;
    tail_old = *tail_;
    next = head_old.value()->next;
    if (head_->raw() == head_old.raw()) {
      if (head_old.value() == tail_old.value()) {
        if (next.value() == NULL) {
          return false;
        }
        AtomicPointer<Node*> tail_new(next.value(), tail_old.aba() + 1);
        tail_->cas(tail_old, tail_new);
      } else {
        if (retries >= max_retries_) {
          random_index = 0;
        } else {
          random_index = pseudorand() % quasi_factor_;
        }
        retries++;

        Node *node = next.value();
        if (random_index == 0) {
          while (node != NULL && node->deleted == true) {
            AtomicPointer<Node*> head_new(node, head_old.aba() + 1);
            if (!head_->cas(head_old, head_new) || node == tail_old.value()) {
              goto TOP_WHILE;
            }
            head_old = head_new;
            next = head_old.value()->next;
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
          for (i = 0; i < random_index && node->next.value() != NULL; ++i) {
            node = node->next.value();
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

#endif  // SCAL_DATASTRUCTURES_RANDOM_DEQUEUE_QUEUE_H_
