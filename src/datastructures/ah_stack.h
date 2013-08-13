#ifndef SCAL_DATASTRUCTURES_AH_STACK_H_
#define SCAL_DATASTRUCTURES_AH_STACK_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/stack.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

namespace ahs_details {
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
class AHStack : public Stack<T> {
 public:
  explicit AHStack(uint64_t num_threads);
  bool push(T item);
  bool pop(T *item);

 private:
  typedef ahs_details::Item<T> Item;

  // The number of threads.
  uint64_t num_threads_;
  // The thread-local clocks.
  std::atomic<uint64_t>* *clocks_;
  std::atomic<uint64_t> *clock_;
  std::atomic<uint64_t> *dequeue_clock_;
  // The thread-local queues, implemented as arrays of size BUFFERSIZE. At the
  // moment buffer overflows are not considered.
  std::atomic<Item*> **queues_;
  // The insert pointers of the thread-local queues.
  std::atomic<uint64_t>* *insert_;
  // The pointers for the emptiness check.
  uint64_t* *emptiness_check_pointers_;

  bool find_youngest_item(Item* *result, uint64_t start, uint64_t *timestamp);
  uint64_t get_youngest_item(uint64_t thread_id, Item* *result);

};

template<typename T>
AHStack<T>::AHStack(uint64_t num_threads) {
  num_threads_ = num_threads;

  clocks_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

  queues_ = static_cast<std::atomic<Item*>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), scal::kCachePrefetch * 2));

  insert_ = static_cast<std::atomic<uint64_t>**>(
      scal::calloc_aligned(num_threads_, sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));


  emptiness_check_pointers_ = static_cast<uint64_t**>(
      scal::calloc_aligned(num_threads_, sizeof(uint64_t*), scal::kCachePrefetch * 2));

  for (int i = 0; i < num_threads_; i++) {
    queues_[i] = static_cast<std::atomic<Item*>*>(
        calloc(BUFFERSIZE, sizeof(std::atomic<Item*>)));

    Item *new_item = scal::get<Item>(scal::kCachePrefetch);
    new_item->timestamp.store(0, std::memory_order_release);
    new_item->data.store(0, std::memory_order_release);
    new_item->taken.store(1, std::memory_order_release);
    queues_[i][0].store(new_item, std::memory_order_release);

    insert_[i] = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
    insert_[i]->store(1);
    clocks_[i] = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
    clocks_[i]->store(1);

    emptiness_check_pointers_[i] = static_cast<uint64_t*> (
        scal::calloc_aligned(num_threads_, sizeof(uint64_t), scal::kCachePrefetch *2));
  }

  clock_ = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
  clock_->store(UINT64_MAX - 1);
//  clock_->store(1);
}

inline uint64_t max(uint64_t x, uint64_t y) {
  if (x > y) {
    return x;
  }

  return y;
}

template<typename T>
bool AHStack<T>::push(T element) {

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
  // 2) Create a new item which stores the element and the current time as
  //    time stamp.
  // 3) Insert the new item into the thread-local queue.
  // 4) Set the thread-local time to the current time + 1.

  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  // Make sure that have no buffer overflow.
//  assert(insert_[thread_id]->load(std::memory_order_relaxed) < BUFFERSIZE);

  // 1) Determine the latest time of all clocks, which is considered as the
  //    current time.
//  uint64_t latest_time = 0;
//
//  for (int i = 0; i < num_threads_; i++) {
//    latest_time = max(latest_time, clocks_[i]->load(std::memory_order_acquire));
//  }
//
//  // 2) Set the thread-local time to the current time + 1.
//  clocks_[thread_id]->store(latest_time + 1, std::memory_order_release);

//  uint64_t latest_time = clock_->fetch_add(1);
  uint64_t latest_time = get_hwtime();
  // 3) Create a new item which stores the element and the current time as
  //    time stamp.
  Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
  new_item->timestamp.store(latest_time, std::memory_order_release);
  new_item->data.store(element, std::memory_order_release);
  new_item->taken.store(0, std::memory_order_release);

  // 4) Insert the new item into the thread-local queue.
  uint64_t insert = insert_[thread_id]->load(std::memory_order_acquire);

  // Fix the insert pointer.
  for (; insert % ABAOFFSET > 1; insert--) {
    if (queues_[thread_id][(insert %ABAOFFSET) - 1].load(std::memory_order_acquire)
        ->taken.load(std::memory_order_acquire) == 0) {
      break;
    }
  }

  queues_[thread_id][insert % ABAOFFSET].store(new_item, std::memory_order_release);
  insert_[thread_id]->store(insert + 1 + ABAOFFSET, std::memory_order_release);
  return true;
}

template<typename T>
bool AHStack<T>::pop(T *element) {

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

  uint64_t timestamp = clock_->load() + 1;
//  uint64_t timestamp = UINT64_MAX;

  Item *oldest;
  uint64_t start = pseudorand() % num_threads_;
  uint64_t counter = 0;
  while (true) {
    if (!find_youngest_item(&oldest, start, &timestamp)) {
      *element = (T)NULL;
      return false;
    } else if (oldest != NULL) {
      *element = oldest->data.load();
      return true;
    }
  }
}

// Returns the oldest not-taken item from all thread-local queues. The 
// thread-local queues are accessed exactly once, it an older element is 
// enqueued in the meantime, then it is ignored.
template<typename T>
bool AHStack<T>::find_youngest_item(Item* *result, uint64_t start, uint64_t *t) {

  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  uint64_t *emptiness_check_pointers = emptiness_check_pointers_[thread_id];
  bool empty = true;
  uint64_t remove_index = -1;
  *result = NULL;
  uint64_t index = -1;
  uint64_t timestamp = 0;
  uint64_t old_remove_index = 0;

  for (uint64_t i = 0; i < num_threads_; i++) {

    Item* item;
    uint64_t remove;
    uint64_t old_remove_index_item = insert_[(start + i) % num_threads_]->load();
    remove = get_youngest_item((start + i) % num_threads_, &item);
    if (item != NULL) {
      empty = false;
      uint64_t item_timestamp = item->timestamp.load(std::memory_order_acquire);
      if (timestamp < item_timestamp) {
        *result = item;
        index = (start + i) % num_threads_;
        timestamp = item_timestamp;
        remove_index = remove;
        old_remove_index = old_remove_index_item;
      } 
      if (*t <= timestamp) {
        uint64_t expected = 0;
        if ((*result)->taken.compare_exchange_weak(
                expected, 1, 
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
          insert_[index]->compare_exchange_weak(
              old_remove_index, remove_index,
              std::memory_order_acq_rel, std::memory_order_relaxed);
          return true;
        }
      }
    } else {
      if (emptiness_check_pointers[(start + i) % num_threads_] != remove) {
        empty = false;
        emptiness_check_pointers[(start + i) % num_threads_] = remove;
      }
    }
  }
  if (*result != NULL) {
    uint64_t expected = 0;
    if ((*result)->taken.compare_exchange_weak(
            expected, 1, 
            std::memory_order_acq_rel, std::memory_order_relaxed)) {
      insert_[index]->compare_exchange_weak(
          old_remove_index, remove_index,
          std::memory_order_acq_rel, std::memory_order_relaxed);
      return true;
    } else {
      *t = (*result)->timestamp.load() + 1;
      *result = NULL;
    }
  }

  return !empty;
}

// Returns the oldest not-taken item from the thread-local queue indicated by
// thread_id.
template<typename T>
uint64_t AHStack<T>::get_youngest_item(uint64_t thread_id, Item* *result) {

  uint64_t remove_index = insert_[thread_id]->load();
  uint64_t remove = remove_index - 1;

  *result = NULL;
  if (remove % ABAOFFSET < 1) {
    return remove_index;
  }

  while (remove % ABAOFFSET >= 1) {
    *result = queues_[thread_id][remove % ABAOFFSET].load(std::memory_order_acquire);
    if ((*result)->taken.load(std::memory_order_acquire) == 0) {
      break;
    }
    remove--;
  }

  if (remove % ABAOFFSET < 1) {
    *result = NULL;
    return remove_index;
  }
  return remove;
}

#endif  // SCAL_DATASTRUCTURES_AH_QUEUE_H_
