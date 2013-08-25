#ifndef SCAL_DATASTRUCTURES_TS_QUEUE_H_
#define SCAL_DATASTRUCTURES_TS_QUEUE_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/queue.h"
#include "datastructures/ts_timestamp.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

//////////////////////////////////////////////////////////////////////
// The base TSQueueBuffer class.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TSQueueBuffer {
  public:
    virtual void insert_element(T element, uint64_t timestamp) = 0;
    virtual bool try_remove_oldest(T *element, uint64_t *threshold) = 0;
};

//////////////////////////////////////////////////////////////////////
// A TSQueueBuffer based on thread-local arrays.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TLArrayQueueBuffer : public TSQueueBuffer<T> {
  private:
    typedef struct Item {
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    // The thread-local queues, implemented as arrays of size BUFFERSIZE. 
    // At the moment buffer overflows are not considered.
    std::atomic<Item*> **buckets_;
    // The insert pointers of the thread-local queues.
    std::atomic<uint64_t>* *insert_;
    // The remove pointers of the thread-local queues.
    std::atomic<uint64_t>* *remove_;
    // The pointers for the emptiness check.
    uint64_t* *emptiness_check_pointers_;

    // Returns the oldest not-taken item from the thread-local queue 
    // indicated by thread_id.
    uint64_t get_oldest_item(uint64_t thread_id, Item* *result) {

      uint64_t insert = insert_[thread_id]->load();
      uint64_t remove = remove_[thread_id]->load();

      // If the thread-local queue is empty, no element is returned.
      if (insert == remove) {
        *result = NULL;
      } else {
        *result = buckets_[thread_id][remove].load();
      }
      return remove;
    }

  public:
    TLArrayQueueBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      buckets_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      insert_ = static_cast<std::atomic<uint64_t>**>(
          scal::calloc_aligned(num_threads_, 
            sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

      remove_ = static_cast<std::atomic<uint64_t>**>(
          scal::calloc_aligned(num_threads_, 
            sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));


      emptiness_check_pointers_ = static_cast<uint64_t**>(
          scal::calloc_aligned(num_threads_, sizeof(uint64_t*), 
            scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {
        buckets_[i] = static_cast<std::atomic<Item*>*>(
            calloc(BUFFERSIZE, sizeof(std::atomic<Item*>)));

        insert_[i] = scal::get<std::atomic<uint64_t>>(
            scal::kCachePrefetch);
        insert_[i]->store(0);

        emptiness_check_pointers_[i] = static_cast<uint64_t*> (
            scal::calloc_aligned(num_threads_, sizeof(uint64_t), 
              scal::kCachePrefetch *2));
      }

      for (int i = 0; i < num_threads_; i++) {
        remove_[i] = scal::get<std::atomic<uint64_t>>(
            scal::kCachePrefetch * 2);
        remove_[i]->store(0);
      }
    }

    void insert_element(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->timestamp.store(timestamp, std::memory_order_release);
      new_item->data.store(element, std::memory_order_release);

      // 4) Insert the new item into the thread-local queue.
      uint64_t insert = insert_[thread_id]->load();
      buckets_[thread_id][insert].store(new_item);
      insert_[thread_id]->store(insert + 1);
    };

    /////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////
    bool try_remove_oldest(T *element, uint64_t *threshold) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      uint64_t *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
      // The remove_index stores the index where the element is stored in
      // a thread-local buffer.
      uint64_t buffer_array_index = -1;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the oldest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the oldest item found until now.
      uint64_t timestamp = UINT64_MAX;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      uint64_t old_remove_index = 0;
      bool all_full = true;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        Item* item;
        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local 
        // buffer.
        if (tmp_buffer_index == 0) {
          continue;
        }
        // We get the oldest element from that thread-local buffer.
        uint64_t tmp_buffer_array_index = 
          get_oldest_item(tmp_buffer_index, &item);
        // If we found an element, we compare it to the oldest element 
        // we have found until now.
        if (item != NULL) {
          empty = false;
          uint64_t item_timestamp = 
            item->timestamp.load(std::memory_order_acquire);
          if (timestamp > item_timestamp) {
            // We found a new oldest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            buffer_array_index = tmp_buffer_array_index;
          } 
          // Check if we can remove the element immediately.
//          if (*threshold > timestamp) {
//            if (remove_[buffer_index]->compare_exchange_weak(
//                buffer_array_index, buffer_array_index + 1)) {
//              *element = result->data.load();
//              return true;
//            }
//          }
        } else {
          // No element was found, work on the emptiness check.
          all_full = false;
          if (emptiness_check_pointers[tmp_buffer_index] 
              != tmp_buffer_array_index) {
            empty = false;
            emptiness_check_pointers[tmp_buffer_index] = 
              tmp_buffer_array_index;
          }
        }
      }
      if (result != NULL) {
        if (timestamp <= *threshold || all_full) {
          if (remove_[buffer_index]->compare_exchange_weak(
              buffer_array_index, buffer_array_index + 1)) {
            *element = result->data.load();
            return true;
          }
        }
        // We were not able to remove the oldest element. However, in
        // later invocations we can remove any element younger than the
        // one we found in this iteration. Therefore the time stamp of
        // the element we found in this round will be the threshold for
        // the next round.
        *threshold = result->timestamp.load();
      }

      *element = (T)NULL;
      return !empty;
    }
};

//////////////////////////////////////////////////////////////////////
// A TSQueueBuffer based on thread-local linked lists.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TLLinkedListQueueBuffer : public TSQueueBuffer<T> {
  private:
    typedef struct Item {
      std::atomic<Item*> next;
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    // The thread-local queues, implemented as arrays of size BUFFERSIZE. 
    // At the moment buffer overflows are not considered.
    std::atomic<Item*> **insert_;
    std::atomic<Item*> **remove_;
    // The pointers for the emptiness check.
    Item** *emptiness_check_pointers_;

    void *get_aba_free_pointer(void *pointer) {
      uint64_t result = (uint64_t)pointer;
      result &= 0xfffffffffffffff8;
      return (void*)result;
    }

    void *add_next_aba(void *pointer, void *old, uint64_t increment) {
      uint64_t aba = (uint64_t)old;
      aba += increment;
      aba &= 0x7;
      uint64_t result = (uint64_t)pointer;
      result = (result & 0xfffffffffffffff8) | aba;
      return (void*)((result & 0xffffffffffffff8) | aba);
    }

  public:
    /////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////
    TLLinkedListQueueBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      insert_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      remove_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {

        insert_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch));

        remove_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch);
        new_item->timestamp.store(0);
        new_item->data.store(0);
        new_item->next.store(NULL);
        insert_[i]->store(new_item);
        remove_[i]->store(new_item);

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 2));
      }
    }

    /////////////////////////////////////////////////////////////////
    // insert_element
    /////////////////////////////////////////////////////////////////
    void insert_element(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->timestamp.store(timestamp);
      new_item->data.store(element);
      new_item->next.store(NULL);

      Item* old_insert = insert_[thread_id]->load();
      old_insert->next.store(new_item);
      insert_[thread_id]->store(new_item);
    };

    /////////////////////////////////////////////////////////////////
    // try_remove_oldest
    /////////////////////////////////////////////////////////////////
    bool try_remove_oldest(T *element, uint64_t *threshold) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the oldest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the oldest item found until now.
      uint64_t timestamp = 0;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_remove = NULL;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local 
        // buffer.
        Item* tmp_remove = remove_[tmp_buffer_index]->load();
        Item* tmp_insert = insert_[tmp_buffer_index]->load();
        Item* item = 
          ((Item*)get_aba_free_pointer(tmp_remove))->next.load();
        // We get the oldest element from that thread-local buffer.
        // If we found an element, we compare it to the oldest element 
        // we have found until now.
        if (get_aba_free_pointer(tmp_remove) != tmp_insert) {
          assert(item != NULL);
          empty = false;
          uint64_t item_timestamp = 
            item->timestamp.load(std::memory_order_acquire);
          if (timestamp < item_timestamp) {
            // We found a new oldest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_remove = tmp_remove;
          } 
        } else {
          // No element was found, work on the emptiness check.
          if (emptiness_check_pointers[tmp_buffer_index] 
              != tmp_remove) {
            empty = false;
            emptiness_check_pointers[tmp_buffer_index] = 
              tmp_remove;
          }
        }
      }
      if (result != NULL) {
        if (timestamp <= *threshold) {
          if (remove_[buffer_index]->load() == old_remove) {
          if (remove_[buffer_index]->compare_exchange_weak(
                old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
            *element = result->data.load(std::memory_order_acquire);
            return true;
          }
          }
        }
        *threshold = result->timestamp.load();
      }
      *element = (T)NULL;
      return !empty;
    }
};

template<typename T>
class TSQueue : public Queue<T> {
 private:
  TSQueueBuffer<T> *buffer_;
  TimeStamp *timestamping_;
 public:
  explicit TSQueue
    (TSQueueBuffer<T> *buffer, TimeStamp *timestamping) 
    : buffer_(buffer), timestamping_(timestamping) {
  }
  bool enqueue(T element);
  bool dequeue(T *element);
};

template<typename T>
bool TSQueue<T>::enqueue(T element) {
  uint64_t timestamp = timestamping_->get_timestamp();
  buffer_->insert_element(element, timestamp);
  return true;
}

template<typename T>
bool TSQueue<T>::dequeue(T *element) {
  uint64_t threshold;
  threshold = 0;
  while (buffer_->try_remove_oldest(element, &threshold)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_QUEUE_H_
