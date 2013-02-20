#include <stdlib.h>
#include "element.h"
#include <inttypes.h>
#include "analyzer.h"
#include <stdio.h>

struct Node {

  Element* element;
  Node* prev;
  Node* next;
};

// Assumption: elements are ordered by their order.
Result* calculate_age(Element** elements, int num_ops) {

  // Initialize the result
  Result* result = new Result();
  result->max_costs = 0;
  result->num_ops = 0;
  result->total_costs = 0;

  // Create the doubly-linked list.
  Node* head = new Node;
  head->element = NULL;

  Node* last = head;
  int next_imaginary_value = -2;
  
  for (int i = 0; i < num_ops; i++) {

    // If the element is a null-return remove operation, then replace it by an
    // imaginary insert/remove pair with a unique value. Imaginary operations
    // have negative values.
    if (elements[i]->is_null_return()) {
      Element* imaginary_insert = new Element();
      imaginary_insert->initialize(
          elements[i]->order(), Element::INSERT, next_imaginary_value);
      elements[i]->set_imaginary_value(next_imaginary_value);
      next_imaginary_value--;

      Node* node = new Node();
      node->element = imaginary_insert;
      node->prev = last;
      last->next = node;
      last = node;
    }

    Node* node = new Node();
    node->element = elements[i];
    node->prev = last;
    last->next = node;
    last = node;
  }

  head->prev = last;
  last->next = head;

  Node* next = head->next;

  while (next != head) {
    
    if (next->element->type() == Element::INSERT) {
      // Do nothing for insert operations because they do not have any costs.
      next = next->next;
    } else {

      // Calculate the costs of the operation.
      uint64_t costs = 0;
      Node* tmp = head->next;
      while (tmp != head) {
        if (tmp->element->type() == Element::REMOVE) {
          // Remove operations do not create any costs, do nothing.
          tmp = tmp->next;
        } else if (tmp->element->value() == next->element->value()) {
          // We found the matching insert operation, so we can stop the search.
          // But first we remove the insert operation from the list.
          tmp->prev->next = tmp->next;
          tmp->next->prev = tmp->prev;
          //TODO remove tmp from the list.
          break;
        } else {
          // Costs increase by one.

//          if (next->element->is_null_return()) {
//            printf("-%d precedes -%d but should not\n", next->element->value(), tmp->element->value());
//            exit(0);
//          }
          costs++;
          tmp = tmp->next;
        }
      }

      // Add the costs.
      result->total_costs += costs;
      result->num_ops++;
      if (costs > result->max_costs) {
        result->max_costs = costs;
      }

      // Remove the operation from the list.
      next->prev->next = next->next;
      next->next->prev = next->prev;

      // Set the pointer to the next operation.
      next = next->next;
    }
  }

  result->avg_costs = result->total_costs;
  result->avg_costs /= (double)result->num_ops;
  return result;
}
