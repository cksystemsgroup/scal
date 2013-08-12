
#ifndef SCAL_DATASTRUCTURES_AH_QUEUE_H_
#define SCAL_DATASTRUCTURES_AH_QUEUE_H_

#include <stdint.h>
#include <assert.h>
#include <atomic>

#include "datastructures/queue.h"
#include "util/threadlocals.h"
#include "util/random.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"

#define BUFFERSIZE 1000000

#define CACHE_FACTOR_INSERT 16
#define CACHE_FACTOR_QUEUE 16
#define CACHE_FACTOR_ITEM 16

namespace ahs_details {
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
class AHStack : public Queue<T> {
 public:
  explicit AHStack(uint64_t num_threads);
  bool enqueue(T item);
  bool dequeue(T *item);

 private:
  // The number of threads.
  uint64_t num_threads_;
  // The thread-local clocks.
  std::atomic<uint64_t> *clocks_;
  // A global clock if we want to use a clock based on fetch-and-inc.
  std::atomic<uint64_t> clock_;
  // The thread-local queues, implemented as arrays of size BUFFERSIZE. At the
  // moment buffer overflows are not considered.
  std::atomic<ahs_details::Item<T>*> **queues_;
  // The insert pointers of the thread-local queues.
  std::atomic<uint64_t> *insert_;

  ahs_details::Item<T>* find_youngest_item();
  ahs_details::Item<T>* get_youngest_item(uint64_t index);
  ahs_details::Item<T>* dequeue_younger(uint64_t timestamp);

  inline uint64_t get_insert(uint64_t index) {
    return insert_[index * CACHE_FACTOR_INSERT].load(std::memory_order_acquire);
  }

  inline void set_insert(uint64_t index, uint64_t value) {
    insert_[index * CACHE_FACTOR_INSERT].store(value, std::memory_order_release);
  }

};

template<typename T>
AHStack<T>::AHStack(uint64_t num_threads) {
  num_threads_ = num_threads - 1;

  clocks_ = static_cast<std::atomic<uint64_t>*>(
      calloc(num_threads, sizeof(std::atomic<uint64_t>)));
  queues_ = static_cast<std::atomic<ahs_details::Item<T>*>**>(
      calloc(num_threads * CACHE_FACTOR_QUEUE, sizeof(std::atomic<ahs_details::Item<T>*>*)));

  for (int i = 0; i < num_threads; i++) {
    queues_[i * CACHE_FACTOR_QUEUE] = static_cast<std::atomic<ahs_details::Item<T>*>*>(
        calloc(BUFFERSIZE, sizeof(std::atomic<ahs_details::Item<T>*>)));
  }

  clock_.store(0);

  insert_ = static_cast<std::atomic<uint64_t>*>(calloc(num_threads * CACHE_FACTOR_INSERT, sizeof(std::atomic<uint64_t>)));
}

inline uint64_t max(uint64_t x, uint64_t y) {
  if (x > y) {
    return x;
  }

  return y;
}

template<typename T>
bool AHStack<T>::enqueue(T element) {

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  // 3) Insert the new item into the thread-local queue.
  // 4) Set the thread-local time to the current time + 1.

  uint64_t thread_id = scal::ThreadContext::get().thread_id() - 1;
  // Make sure that have no buffer overflow.
//  assert(insert_[thread_id].load(std::memory_order_relaxed) < BUFFERSIZE);

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.

//  uint64_t latest_time = 0;
//
//  for (int i = 0; i < num_threads_; i++) {
//    latest_time = max(latest_time, clocks_[i].load(std::memory_order_acquire));
//  }

  uint64_t latest_time = get_hwtime();
//  uint64_t latest_time = clock_.fetch_add(1);

  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  
  ahs_details::Item<T> *new_item = scal::tlget_aligned<ahs_details::Item<T>>(scal::kCachePrefetch);

//  ahs_details::Item<T> *new_item = new ahs_details::Item<T>();
  new_item->taken.store(0, std::memory_order_release);
  new_item->timestamp = latest_time;
  new_item->data = element;

  // 3) Insert the new item into the thread-local queue.
  uint64_t insert = get_insert(thread_id);

  // Fix the insert pointer.
  for (; insert >= 1; insert --) {
    if (queues_[thread_id * CACHE_FACTOR_QUEUE][(insert - 1) * CACHE_FACTOR_ITEM].load(std::memory_order_acquire)
        ->taken.load(std::memory_order_acquire) == 0) {
      break;
    }
  }

  queues_[thread_id * CACHE_FACTOR_QUEUE][insert * CACHE_FACTOR_ITEM].store(new_item, std::memory_order_release);
  set_insert(thread_id, insert + 1);

  // 4) Set the thread-local time to the current time + 1.
  clocks_[thread_id].store(latest_time + 1, std::memory_order_release);

  return true;
}

template<typename T>
bool AHStack<T>::dequeue(T *element) {

  // Repeat as long as no element is found
  // 1) Find the youngest element in the queue.
  //    If no element is found, try again.
  // 2) Dequeue any element younger than the youngest element found in the 
  //    first round. The dequeued element either has the same time stamp as
  //    the element found in the first iteration and is therefore also a
  //    potential youngest element, or it was enqueued during or after the
  //    first iteration and therefore overlaps with the current dequeue
  //    operation.

  while (true) {
    ahs_details::Item<T> *youngest = find_youngest_item();

    if (youngest == NULL) {
      // We do not provide an emptiness check at the moment.
      continue;
    }

    ahs_details::Item<T> *item = dequeue_younger(youngest->timestamp);
    if (item != NULL) {
      *element = item->data;
      return true;
    }
  }
}

// Returns the oldest not-taken item from the thread-local queue indicated by
// thread_id.
template<typename T>
ahs_details::Item<T>* AHStack<T>::get_youngest_item(uint64_t thread_id) {

  int64_t remove = get_insert(thread_id) - 1;


  if (remove < 0) {
    return NULL;
  }

  ahs_details::Item<T>* item = queues_[thread_id * CACHE_FACTOR_QUEUE][remove * CACHE_FACTOR_ITEM].load(std::memory_order_acquire);
  while (remove >= 0 && item->taken.load(std::memory_order_acquire) != 0) {
    remove--;
    item = queues_[thread_id * CACHE_FACTOR_QUEUE][remove * CACHE_FACTOR_ITEM].load(std::memory_order_acquire);
  }

  if (remove < 0) {
    return NULL;
  }
  return item;
}

// Returns the oldest not-taken item from all thread-local queues. The 
// thread-local queues are accessed exactly once, it an older element is 
// enqueued in the meantime, then it is ignored.
template<typename T>
ahs_details::Item<T>* AHStack<T>::find_youngest_item() {

  ahs_details::Item<T>* result = NULL;
  uint64_t start = pseudorand() % num_threads_;
  for (uint64_t i = 0; i < num_threads_; i++) {
//  for (uint64_t i = 0; i < num_threads_; i++) {

    ahs_details::Item<T>* item = get_youngest_item((start + i) % num_threads_);
    if (result == NULL) {
      result = item;
    } else if (item != NULL) {
      if (result->timestamp < item->timestamp) {
        result = item;
      }
    }
  }

  return result;
}

// Returns the oldest not-taken item from all thread-local queues. The 
// thread-local queues are accessed exactly once, it an older element is 
// enqueued in the meantime, then it is ignored.
template<typename T>
ahs_details::Item<T>* AHStack<T>::dequeue_younger(uint64_t timestamp) {

  ahs_details::Item<T>* result = NULL;

  uint64_t start = pseudorand() % num_threads_;
  for (uint64_t i = 0; i < num_threads_; i++) {

    ahs_details::Item<T>* item = get_youngest_item((start + i) % num_threads_);
    if (item != NULL && item->timestamp >= timestamp) {
      uint64_t expected = 0;
      if (item->taken.compare_exchange_weak(expected, 1,
            std::memory_order_acq_rel, std::memory_order_relaxed)) {
        return item;
      }
    }
  }
  return NULL;
}

#endif  // SCAL_DATASTRUCTURES_AH_QUEUE_H_
