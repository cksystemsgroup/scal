// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// A singly-linked list using a sentinel node that can only be used in
// single-threaded.

#ifndef SCAL_DATASTRUCTURES_SINGLE_DEQUE_H_
#define SCAL_DATASTRUCTURES_SINGLE_DEQUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "datastructures/deque.h"
#include "util/malloc.h"

// A deque implemented as a cyclic doubly-linked list.
template<typename T>
class SingleDeque : public Deque<T> {
 public:
  SingleDeque();
  bool insert_left(T item);
  bool remove_left(T *item);
  bool insert_right(T item);
  bool remove_right(T *item);
  bool is_empty() const;

 private:
  template<typename S>
  struct Node {
    Node *left;
    Node *right;
    S value;
  };

  // Pointer to the sentinal node of the cyclic doubly-linked list. The 
  // "left-most" node will be stored to the right of the sentinal node,
  // the "right-most" node will be stored to the left of the sentinal
  // node.
  Node<T> *list_;

  // ********* -> ************* ->    ->    -> ************** -> ********* 
  // * list_ *    * left_most *    ..    ..    * right_most *    * list_ *
  // ********* <- ************* <-    <-    <- ************** <- *********
};

template<typename T>
SingleDeque<T>::SingleDeque() {
  Node<T> *n = scal::get<Node<T> >(0);
  n->left = n;
  n->right = n;
  list_ = n;
}

template<typename T>
bool SingleDeque<T>::is_empty() const {
  if (list_->left == list_) {
    return true;
  } else {
    return false;
  }
}

template<typename T>
bool SingleDeque<T>::insert_left(T item) {
  // Store the item in a node right to the access node.
  Node<T> *n = scal::tlget<Node<T> >(0);
  n->value = item;
  n->left = list_;
  n->right = list_->right;
  list_->right->left = n;
  list_->right = n;
  assert(list_->left != list_);
  return true;
}

template<typename T>
bool SingleDeque<T>::insert_right(T item) {
  // Store the item in a node left to the access node.
  Node<T> *n = scal::tlget<Node<T> >(0);
  n->value = item;
  n->right = list_;
  n->left = list_->left;
  list_->left->right = n;
  list_->left = n;
  return true;
}

template<typename T>
bool SingleDeque<T>::remove_left(T *item) {
  // Retrieve the item of the left-most node, which is the node to the right
  // of the access node.
  if (list_->left == list_) {
    *item = (T)NULL;
    return false;
  } else {
    *item = list_->right->value;
    list_->right->right->left = list_;
    list_->right = list_->right->right;
    return true;
  }
}

template<typename T>
bool SingleDeque<T>::remove_right(T *item) {
  // Retrieve the item of the right-most node, which is the node to the left
  // of the access node.
  if (list_->left == list_) {
    *item = (T)NULL;
    return false;
  } else {
    *item = list_->left->value;
    list_->left->left->right = list_;
    list_->left = list_->left->left;
    return true;
  }
}

#endif  // SCAL_DATASTRUCTURES_SINGLE_DEQUE_H_
