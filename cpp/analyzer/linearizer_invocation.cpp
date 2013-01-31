#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"
#include "linearizer.h"
#include <limits.h>

int compare_orders_by_start_time(const void* left, const void* right) {
  return (*(Order**) left)->operation->start() > (*(Order**) right)->operation->start();
}

Order** linearize_by_invocation(Operation** ops, int num_ops) {

  Order** result = new Order*[num_ops];
  
  for (int i = 0; i < num_ops; i++) {

    Order* new_order = new Order();
    new_order->operation = ops[i];
    result[i] = new_order;
  }
  
  qsort(result, num_ops, sizeof(Order*), compare_orders_by_start_time);

  for (int i = 0; i < num_ops; i++) {

    result[i]->order = (uint64_t)i;
  }

  return result;
}
