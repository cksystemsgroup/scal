#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "operation.h"

#ifndef LINEARIZER_H
#define LINEARIZER_H

struct Order {
  uint64_t order;
  Operation* operation;
};

Order** linearize_by_min_max(Operation** ops, int num_ops);
Order** linearize_by_invocation(Operation** ops, int num_ops);
Order** linearize_by_min_sum(Operation** ops, int num_ops, Order** order);
Order** linearize_by_lin_point(Operation** ops, int num_ops);
Order** linearize_by_end(Operation** ops, int num_ops);
#endif // LINEARIZER_H
