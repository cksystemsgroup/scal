#ifndef SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H
#define SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H

typedef long Object;
struct RingQueue;

void lcrq_SHARED_OBJECT_INIT();
bool lcrq_enqueue(uint64_t item);
bool lcrq_dequeue(uint64_t *item);

#endif  // SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H
