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
      std::atomic<Item*> next;
      std::atomic<T> data;
      std::atomic<uint64_t> t1;
      std::atomic<uint64_t> t2;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    // The thread-local queues, implemented as arrays of size BUFFERSIZE. 
    // At the moment buffer overflows are not considered.
    std::atomic<Item*> **insert_;
    std::atomic<Item*> **remove_;
    // The pointers for the emptiness check.
    Item** *emptiness_check_pointers_;

    inline void *get_aba_free_pointer(void *pointer) {
      uint64_t result = (uint64_t)pointer;
      result &= 0xfffffffffffffff8;
      return (void*)result;
    }

    inline void *add_next_aba(void *pointer, void *old, uint64_t increment) {
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
    TLArrayQueueBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      insert_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      remove_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        insert_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        remove_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        new_item->t1.store(0);
        new_item->t2.store(0);
        new_item->data.store(0);
        new_item->next.store(NULL);
        insert_[i]->store(new_item);
        remove_[i]->store(new_item);

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }
    }

    /////////////////////////////////////////////////////////////////
    // insert_element
    /////////////////////////////////////////////////////////////////
    void insert_element(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->t1.store(timestamp);
      new_item->t2.store(UINT64_MAX);
      new_item->data.store(element);
      new_item->next.store(NULL);

      Item* old_insert = insert_[thread_id]->load();
      old_insert->next.store(new_item);
      insert_[thread_id]->store(new_item);
      new_item->t2.store(get_hwtime());
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
      // This flag indicates if all thread-local buckets are not empty. 
      // If all thread-local buckets contain at least one element, we do
      // not have to do the second iteration.
      bool all_full = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the oldest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the oldest item found until now.
      uint64_t t1 = 0;
      uint64_t t2 = 0;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_remove = NULL;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        if (tmp_buffer_index == 0) {
          continue;
        }
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
          uint64_t item_t1 = 
            item->t1.load(std::memory_order_acquire);
          uint64_t item_t2 = 
            item->t2.load(std::memory_order_acquire);
          if (t1 < item_t1 && t2 < item_t2) {
            // We found a new oldest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            t1 = item_t1;
            t2 = item_t2;
            old_remove = tmp_remove;
          } 
        } else {
          all_full = false;
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
//        if (timestamp <= *threshold || all_full) {
          if (remove_[buffer_index]->load() == old_remove) {
          if (remove_[buffer_index]->compare_exchange_weak(
                old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
            *element = result->data.load(std::memory_order_acquire);
            return true;
          }
          }
  //      }
        *threshold = result->t1.load();
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

    inline void *get_aba_free_pointer(void *pointer) {
      uint64_t result = (uint64_t)pointer;
      result &= 0xfffffffffffffff8;
      return (void*)result;
    }

    inline void *add_next_aba(void *pointer, void *old, uint64_t increment) {
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
            scal::kCachePrefetch * 4));

      remove_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        insert_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        remove_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        new_item->timestamp.store(0);
        new_item->data.store(0);
        new_item->next.store(NULL);
        insert_[i]->store(new_item);
        remove_[i]->store(new_item);

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
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
      // This flag indicates if all thread-local buckets are not empty. 
      // If all thread-local buckets contain at least one element, we do
      // not have to do the second iteration.
      bool all_full = true;
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
        if (tmp_buffer_index == 0) {
          continue;
        }
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
          all_full = false;
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
        if (timestamp <= *threshold || all_full) {
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
  int64_t* *counter1_;
  int64_t* *counter2_;
  uint64_t num_threads_;
  inline void inc_counter1() {
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    (*counter1_[thread_id])++;
  }
  inline void inc_counter2() {
    uint64_t thread_id = scal::ThreadContext::get().thread_id();
    (*counter2_[thread_id])++;
  }
 public:
  explicit TSQueue
    (TSQueueBuffer<T> *buffer, TimeStamp *timestamping, 
     uint64_t num_threads) 
    : buffer_(buffer), timestamping_(timestamping), num_threads_(num_threads) {
    counter1_ = static_cast<int64_t**>(
        scal::calloc_aligned(num_threads, sizeof(uint64_t*), 
          scal::kCachePrefetch * 4));

    counter2_ = static_cast<int64_t**>(
        scal::calloc_aligned(num_threads, sizeof(uint64_t*), 
          scal::kCachePrefetch * 4));

    for (uint64_t i = 0; i < num_threads; i++) {
      counter1_[i] = scal::get<int64_t>(scal::kCachePrefetch * 4);
      *(counter1_[i]) = 0;
      counter2_[i] = scal::get<int64_t>(scal::kCachePrefetch * 4);
      *(counter2_[i]) = 0;
    }
  }
  bool enqueue(T element);
  bool dequeue(T *element);

  char* ds_get_stats(void) {

    int64_t sum1 = 0;
    int64_t sum2 = 0;

    for (int i = 0; i < num_threads_; i++) {
      sum1 += *counter1_[i];
      sum2 += *counter2_[i];
    }

    char buffer[255] = { 0 };
    uint32_t n = snprintf(buffer,
                          sizeof(buffer),
                          "%ld %ld %ld",
                          sum1, sum2, 
                          sum1 ? (sum2 / sum1) : 0
                          );
    if (n != strlen(buffer)) {
      fprintf(stderr, "%s: error creating stats string\n", __func__);
      abort();
    }
    char *newbuf = static_cast<char*>(calloc(
        strlen(buffer) + 1, sizeof(*newbuf)));
    return strncpy(newbuf, buffer, strlen(buffer));
  }
};

template<typename T>
bool TSQueue<T>::enqueue(T element) {
  uint64_t timestamp = timestamping_->get_timestamp();
  buffer_->insert_element(element, timestamp);
  return true;
}

template<typename T>
bool TSQueue<T>::dequeue(T *element) {
  inc_counter1();
  uint64_t threshold;
  threshold = 0;
  while (buffer_->try_remove_oldest(element, &threshold)) {
    inc_counter2();
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_QUEUE_H_
