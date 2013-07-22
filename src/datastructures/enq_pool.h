#ifndef SCAL_DATASTRUCTURES_AH_QUEUE_H_
#define SCAL_DATASTRUCTURES_AH_QUEUE_H_

#include <stdint.h>
#include <assert.h>
#include <atomic>

#include "datastructures/queue.h"
#include "util/threadlocals.h"
#include "util/random.h"

#define BUFFERSIZE 1000000
#define CACHE_FACTOR 16

namespace enq_details {
  // An item contains the element, a time stamp when the element was enqueued,
  // and a flag which indicates if the element has already been dequeued.
  template<typename T>
    struct Item {
      T data;
      uint64_t timestamp;
      std::atomic<uint64_t> taken;
    };
}

template<typename T>
class ENQQueue : public Queue<T> {
 public:
  explicit ENQQueue(uint64_t num_threads);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  // The number of threads.
  uint64_t num_threads_;
  // The thread-local clocks.
  std::atomic<uint64_t> *clocks_;
  // The thread-local queues, implemented as arrays of size BUFFERSIZE. At the
  // moment buffer overflows are not considered.
  std::atomic<enq_details::Item<T>*> **queues_;
  // The insert pointers of the thread-local queues.
  std::atomic<uint64_t> *insert_;
  // The remove pointers of the thread-local queues.
  std::atomic<uint64_t> *remove_;

  enq_details::Item<T>* find_oldest_item();
  enq_details::Item<T>* get_oldest_item(uint64_t index);

};

template<typename T>
ENQQueue<T>::ENQQueue(uint64_t num_threads) {
  num_threads_ = num_threads;

  clocks_ = static_cast<std::atomic<uint64_t>*>(
      calloc(num_threads * CACHE_FACTOR, sizeof(std::atomic<uint64_t>)));
  queues_ = static_cast<std::atomic<enq_details::Item<T>*>**>(
      calloc(num_threads, sizeof(std::atomic<enq_details::Item<T>*>*)));

  for (int i = 0; i < num_threads; i++) {
    queues_[i] = static_cast<std::atomic<enq_details::Item<T>*>*>(
        calloc(BUFFERSIZE, sizeof(std::atomic<enq_details::Item<T>*>)));
  }

  insert_ = static_cast<std::atomic<uint64_t>*>(calloc(num_threads * CACHE_FACTOR
        , sizeof(std::atomic<uint64_t>)));
  remove_ = static_cast<std::atomic<uint64_t>*>(calloc(num_threads * CACHE_FACTOR
        , sizeof(std::atomic<uint64_t>)));
}

inline uint64_t max(uint64_t x, uint64_t y) {
  if (x > y) {
    return x;
  }

  return y;
}

template<typename T>
bool ENQQueue<T>::enqueue(T element) {

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  // 3) Insert the new item into the thread-local queue.
  // 4) Set the thread-local time to the current time + 1.

  uint64_t thread_id = scal::ThreadContext::get().thread_id() - 1;
  // Make sure that have no buffer overflow.
  //assert(insert_[thread_id * CACHE_FACTOR].load(std::memory_order_relaxed) < BUFFERSIZE);

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  //uint64_t latest_time = clocks_[thread_id * CACHE_FACTOR].load(std::memory_order_relaxed);

  //for (int i = 0; i < 1; i++) {
//    latest_time = max(latest_time, clocks_[i * CACHE_FACTOR].load(std::memory_order_acquire));
  //  latest_time = max(latest_time, clocks_[(pseudorand() % (num_threads_/2)) * CACHE_FACTOR].load(std::memory_order_relaxed));
  //}

  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  enq_details::Item<T> *new_item = new enq_details::Item<T>();
  //new_item->taken.store(0, std::memory_order_release);
  new_item->taken.store(0, std::memory_order_relaxed);
  //new_item->timestamp = latest_time;
  new_item->data = element;

  // 3) Insert the new item into the thread-local queue.
  uint64_t insert = insert_[thread_id * CACHE_FACTOR].load(std::memory_order_relaxed);
  //queues_[thread_id][insert].store(new_item, std::memory_order_release);
  queues_[thread_id][insert].store(new_item, std::memory_order_relaxed);
  //insert_[thread_id * CACHE_FACTOR].store(insert + 1, std::memory_order_release);
  insert_[thread_id * CACHE_FACTOR].store(insert + 1, std::memory_order_relaxed);

  // 4) Set the thread-local time to the current time + 1.
  // clocks_[thread_id * CACHE_FACTOR].store(latest_time + 1, std::memory_order_release);
  //clocks_[thread_id * CACHE_FACTOR].store(latest_time + 1, std::memory_order_relaxed);

  return true;
}

template<typename T>
bool ENQQueue<T>::dequeue(T *element) {

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

  while (true) {
    enq_details::Item<T> *oldest = find_oldest_item();
    if (oldest == NULL) {
      // We do not provide an emptiness check at the moment.
      continue;
    }
//    enq_details::Item<T> *check = find_oldest_item();
//    if (check == NULL) {
//      continue;
//    }
//    if (oldest->timestamp != check->timestamp) {
//      continue;
//    }
    uint64_t expected = 0;
    if (oldest->taken.compare_exchange_strong(expected, 1,
          std::memory_order_acq_rel, std::memory_order_relaxed)) {

//      oldest->taken.store(1, std::memory_order_relaxed);
      *element = oldest->data;
      return true;
    }
  }
}

// Returns the oldest not-taken item from the thread-local queue indicated by
// thread_id.
template<typename T>
enq_details::Item<T>* ENQQueue<T>::get_oldest_item(uint64_t thread_id) {

  uint64_t insert = insert_[thread_id * CACHE_FACTOR].load(std::memory_order_relaxed);
  uint64_t remove = remove_[thread_id * CACHE_FACTOR].load(std::memory_order_relaxed);

  // If the thread-local queue is empty, no element is returned.
  if (insert == remove) {
    return NULL;
  }

  enq_details::Item<T>* result = NULL;

  uint64_t i;
  for (i = remove; i < insert; i++) {

    enq_details::Item<T>* item = queues_[thread_id][i].load(std::memory_order_relaxed);
    // Check if the item has already been taken. Taken items are ignored.
//    if (item->taken.load(std::memory_order_acquire) == 0) {
    if (item->taken.load(std::memory_order_relaxed) == 0) {
      result = item;
      break;
    }
  }

  // Adjust the remove pointer of the thread-local queue. This is only an optimization.
  if (i != remove) {
//    remove_[thread_id * CACHE_FACTOR].compare_exchange_weak(remove, i, 
//        std::memory_order_relaxed, std::memory_order_relaxed);
    remove_[thread_id * CACHE_FACTOR].store(i, std::memory_order_relaxed);
  }

  return result;
}

// Returns the oldest not-taken item from all thread-local queues. The 
// thread-local queues are accessed exactly once, it an older element is 
// enqueued in the meantime, then it is ignored.
template<typename T>
enq_details::Item<T>* ENQQueue<T>::find_oldest_item() {

  uint64_t thread_id = pseudorand(); 
  enq_details::Item<T>* result = NULL;

//  for (uint64_t i = 0; i < num_threads_; i++) {
  for (uint64_t i = 0; i < 12; i++) {

    enq_details::Item<T>* item = get_oldest_item(pseudorand() % (num_threads_));
////    if (result == NULL) {
//      result = item;
//    } else if (item != NULL) {
//      return result;
      if (item != NULL) return item;
//      if (result->timestamp > item->timestamp) {
//        result = item;
//      }
//    }
  }

  return result;
}

#endif  // SCAL_DATASTRUCTURES_AH_QUEUE_H_
