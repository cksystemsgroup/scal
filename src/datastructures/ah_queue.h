#ifndef SCAL_DATASTRUCTURES_AH_QUEUE_H_
#define SCAL_DATASTRUCTURES_AH_QUEUE_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/queue.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"

#define BUFFERSIZE 1000000

namespace ah_details {
  // An item contains the element, a time stamp when the element was enqueued,
  // and a flag which indicates if the element has already been dequeued.
  template<typename T>
    struct Item {
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
      std::atomic<uint64_t> taken;
    };
}

template<typename T>
class AHQueue : public Queue<T> {
 public:
  explicit AHQueue(uint64_t num_threads);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  typedef ah_details::Item<T> Item;

  // The number of threads.
  uint64_t num_threads_;
  // The thread-local clocks.
  std::atomic<uint64_t>* *clocks_;
  // The thread-local queues, implemented as arrays of size BUFFERSIZE. At the
  // moment buffer overflows are not considered.
  std::atomic<Item*> **queues_;
  // The insert pointers of the thread-local queues.
  std::atomic<uint64_t>* *insert_;
  // The remove pointers of the thread-local queues.
  std::atomic<uint64_t>* *remove_;

  uint64_t find_oldest_item(Item* *result);
  Item* get_oldest_item(uint64_t index);

};

template<typename T>
AHQueue<T>::AHQueue(uint64_t num_threads) {
  num_threads_ = num_threads;

  clocks_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

  queues_ = static_cast<std::atomic<Item*>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), scal::kCachePrefetch * 2));

  insert_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

  remove_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

  for (int i = 0; i < num_threads_; i++) {
    queues_[i] = static_cast<std::atomic<Item*>*>(
        calloc(BUFFERSIZE, sizeof(std::atomic<Item*>)));

    insert_[i] = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
    remove_[i] = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
    clocks_[i] = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
  }

//  clocks_ = static_cast<std::atomic<uint64_t>*>(
//      calloc(num_threads, sizeof(std::atomic<uint64_t>)));
//  queues_ = static_cast<std::atomic<Item*>**>(
//      calloc(num_threads, sizeof(std::atomic<Item*>*)));
//
//  for (int i = 0; i < num_threads; i++) {
//    queues_[i] = static_cast<std::atomic<Item*>*>(
//        calloc(BUFFERSIZE, sizeof(std::atomic<Item*>)));
//  }
//
//  insert_ = static_cast<std::atomic<uint64_t>*>(calloc(num_threads, sizeof(std::atomic<uint64_t>)));
//  remove_ = static_cast<std::atomic<uint64_t>*>(calloc(num_threads, sizeof(std::atomic<uint64_t>)));
}

inline uint64_t max(uint64_t x, uint64_t y) {
  if (x > y) {
    return x;
  }

  return y;
}

template<typename T>
bool AHQueue<T>::enqueue(T element) {

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  // 3) Insert the new item into the thread-local queue.
  // 4) Set the thread-local time to the current time + 1.

  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  // Make sure that have no buffer overflow.
  assert(insert_[thread_id]->load(std::memory_order_relaxed) < BUFFERSIZE);

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  uint64_t latest_time = 0;

  for (int i = 0; i < num_threads_; i++) {
    latest_time = max(latest_time, clocks_[i]->load(std::memory_order_acquire));
  }

  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  Item *new_item = new Item();
  new_item->taken.store(0, std::memory_order_release);
  new_item->timestamp.store(latest_time, std::memory_order_release);
  new_item->data.store(element, std::memory_order_release);

  // 3) Insert the new item into the thread-local queue.
  uint64_t insert = insert_[thread_id]->load(std::memory_order_relaxed);
  queues_[thread_id][insert].store(new_item, std::memory_order_release);
  insert_[thread_id]->store(insert + 1, std::memory_order_release);

  // 4) Set the thread-local time to the current time + 1.
  clocks_[thread_id]->store(latest_time + 1, std::memory_order_release);

  return true;
}

template<typename T>
bool AHQueue<T>::dequeue(T *element) {

  // Repeat as long as no element is found
  // 1) Find the oldest element in the queue.
  //    If no element is found, try again.
  // 2) Check if in the meantime an even older element has been inserted into
  //    a thread-local queue. If this is not the case, then there exists a
  //    linearization point where the oldest element really is the oldest
  //    element in the queue.
  //    If no element is found, then the oldest element has already been 
  //    removed from the queue and we have to find a new oldest element.
  // 3) Try to mark the oldest element as taken. I the marking succeeds,
  //    then we successfully dequeued the oldest element. Otherwise we
  //    have to try again to find a new oldest element.

  Item *oldest;
  uint64_t index = find_oldest_item(&oldest);
  while (true) {
    if (oldest == NULL) {
      index = find_oldest_item(&oldest);
      // We do not provide an emptiness check at the moment.
      continue;
    }
    Item *check;
    uint64_t check_index = find_oldest_item(&check);
    if (check == NULL) {
      index = find_oldest_item(&oldest);
      continue;
    }
    if (oldest->timestamp != check->timestamp) {
      oldest = check;
      index = check_index;
      continue;
    }
    uint64_t expected = 0;
    if (oldest->taken.compare_exchange_strong(expected, 1,
          std::memory_order_acq_rel, std::memory_order_relaxed)) {
      remove_[index]->fetch_add(1);
      *element = oldest->data.load(std::memory_order_acquire);
      return true;
    }
    
    index = find_oldest_item(&oldest);
  }
}

// Returns the oldest not-taken item from all thread-local queues. The 
// thread-local queues are accessed exactly once, it an older element is 
// enqueued in the meantime, then it is ignored.
template<typename T>
uint64_t AHQueue<T>::find_oldest_item(Item* *result) {

  *result = NULL;
  uint64_t index = -1;

  for (uint64_t i = 0; i < num_threads_; i++) {

    Item* item = get_oldest_item(i);
    if (*result == NULL) {
      *result = item;
      index = i;
    } else if (item != NULL) {
      if ((*result)->timestamp.load(std::memory_order_acquire) > item->timestamp.load(std::memory_order_acquire)) {
        *result = item;
        index = i;
      }
    }
  }

  return index;
}

// Returns the oldest not-taken item from the thread-local queue indicated by
// thread_id.
template<typename T>
ah_details::Item<T>* AHQueue<T>::get_oldest_item(uint64_t thread_id) {

  uint64_t insert = insert_[thread_id]->load(std::memory_order_relaxed);
  uint64_t remove = remove_[thread_id]->load(std::memory_order_relaxed);

  // If the thread-local queue is empty, no element is returned.
  if (insert == remove) {
    return NULL;
  }

  Item* result = NULL;

  uint64_t i;
  for (i = remove; i < insert; i++) {

    Item* item = queues_[thread_id][i].load(std::memory_order_relaxed);
    // Check if the item has already been taken. Taken items are ignored.
    if (item->taken.load(std::memory_order_acquire) == 0) {
      result = item;
      break;
    }
  }

  // Adjust the remove pointer of the thread-local queue. This is only an optimization.
//  if (i != remove) {
//    remove_[thread_id]->compare_exchange_weak(remove, i, 
//        std::memory_order_relaxed, std::memory_order_relaxed);
//  }

  return result;
}

#endif  // SCAL_DATASTRUCTURES_AH_QUEUE_H_
