#ifndef SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H
#define SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H

typedef long Object;
struct RingQueue;

void lcrq_SHARED_OBJECT_INIT();
Object lcrq_dequeue();
void lcrq_enqueue(Object arg);

#endif  // SRC_DATASTRUCTURES_LCRQ_ORIGINAL_H
