#ifndef SCAL_DATASTRUCTURES_TS_STACK_H_
#define SCAL_DATASTRUCTURES_TS_STACK_H_

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
#include "util/random.h"

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

//////////////////////////////////////////////////////////////////////
// The base TimeStamp class.
//////////////////////////////////////////////////////////////////////
class TimeStamp {
 public:
  virtual uint64_t get_timestamp() = 0;
};

//////////////////////////////////////////////////////////////////////
// A TimeStamp class based a stuttering counter which requires no
// AWAR or RAW synchronization.
//////////////////////////////////////////////////////////////////////
class StutteringTimeStamp : public TimeStamp {
  private: 
    // The thread-local clocks.
    std::atomic<uint64_t>* *clocks_;
    // The number of threads.
    uint64_t num_threads_;

    inline uint64_t max(uint64_t x, uint64_t y) {
      if (x > y) {
        return x;
      }
      return y;
    }

  public:
    //////////////////////////////////////////////////////////////////////
    // Constructor
    //////////////////////////////////////////////////////////////////////
    StutteringTimeStamp(uint64_t num_threads) 
      : num_threads_(num_threads) {

      clocks_ = static_cast<std::atomic<uint64_t>**>(
          scal::calloc_aligned(num_threads_, 
            sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {
        clocks_[i] = 
          scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
        clocks_[i]->store(1);
      }
    }
  
    //////////////////////////////////////////////////////////////////////
    // Returns a new time stamp.
    //////////////////////////////////////////////////////////////////////
    uint64_t get_timestamp() {

      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      uint64_t latest_time = 0;

      // Find the latest of all thread-local times.
      for (int i = 0; i < num_threads_; i++) {
        latest_time = 
          max(latest_time, clocks_[i]->load(std::memory_order_acquire));
      }

      // 2) Set the thread-local time to the latest found time + 1.
      clocks_[thread_id]->store(
          latest_time + 1, std::memory_order_release);
      // Return the current local time.
      return latest_time + 1;
    }
};

//////////////////////////////////////////////////////////////////////
// A TimeStamp class based on an atomic counter
//////////////////////////////////////////////////////////////////////
class AtomicCounterTimeStamp : public TimeStamp {
  private:
    // Memory for the atomic counter.
    std::atomic<uint64_t> *clock_;

  public:
    AtomicCounterTimeStamp() {
      clock_ = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch);
      clock_->store(UINT64_MAX - 1);
    }
  
    uint64_t get_timestamp() {
      return clock_->fetch_add(1);
    }
};

//////////////////////////////////////////////////////////////////////
// A TimeStamp class based on a hardware counter
//////////////////////////////////////////////////////////////////////
class HardwareTimeStamp : public TimeStamp {
  private:


  public:
    HardwareTimeStamp() {
    }
  
    uint64_t get_timestamp() {
      return get_hwtime();
    }
};

//////////////////////////////////////////////////////////////////////
// The base TSBuffer class.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TSBuffer {
  public:
    virtual void insert_element(T element, uint64_t timestamp) = 0;
    virtual bool try_remove_youngest(T *element, uint64_t *threshold) = 0;
};

//////////////////////////////////////////////////////////////////////
// A TSBuffer based on thread-local arrays.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TLArrayBuffer : public TSBuffer<T> {
  private:
    typedef struct Item {
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
      std::atomic<uint64_t> taken;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    // The thread-local queues, implemented as arrays of size BUFFERSIZE. 
    // At the moment buffer overflows are not considered.
    std::atomic<Item*> **buckets_;
    // The insert pointers of the thread-local queues.
    std::atomic<uint64_t>* *insert_;
    // The pointers for the emptiness check.
    uint64_t* *emptiness_check_pointers_;

    // Returns the oldest not-taken item from the thread-local queue 
    // indicated by thread_id.
    uint64_t get_youngest_item(uint64_t thread_id, Item* *result) {

      uint64_t remove_index = insert_[thread_id]->load();
      uint64_t remove = remove_index - 1;

      *result = NULL;
      if (remove % ABAOFFSET < 1) {
        return remove_index;
      }

      while (remove % ABAOFFSET >= 1) {
        *result = buckets_[thread_id][remove % ABAOFFSET].load(
            std::memory_order_acquire);
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

  public:
    TLArrayBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      buckets_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      insert_ = static_cast<std::atomic<uint64_t>**>(
          scal::calloc_aligned(num_threads_, 
            sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch * 2));


      emptiness_check_pointers_ = static_cast<uint64_t**>(
          scal::calloc_aligned(num_threads_, sizeof(uint64_t*), 
            scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {
        buckets_[i] = static_cast<std::atomic<Item*>*>(
            calloc(BUFFERSIZE, sizeof(std::atomic<Item*>)));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch);
        new_item->timestamp.store(0, std::memory_order_release);
        new_item->data.store(0, std::memory_order_release);
        new_item->taken.store(1, std::memory_order_release);
        buckets_[i][0].store(new_item, std::memory_order_release);

        insert_[i] = scal::get<std::atomic<uint64_t>>(
            scal::kCachePrefetch);
        insert_[i]->store(1);

        emptiness_check_pointers_[i] = static_cast<uint64_t*> (
            scal::calloc_aligned(num_threads_, sizeof(uint64_t), 
              scal::kCachePrefetch *2));
      }
    }

    void insert_element(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->timestamp.store(timestamp, std::memory_order_release);
      new_item->data.store(element, std::memory_order_release);
      new_item->taken.store(0, std::memory_order_release);

      // 4) Insert the new item into the thread-local queue.
      uint64_t insert = insert_[thread_id]->load(
          std::memory_order_acquire);

      // Fix the insert pointer.
      for (; insert % ABAOFFSET > 1; insert--) {
        if (buckets_[thread_id][(insert %ABAOFFSET) - 1].load(
              std::memory_order_acquire)
            ->taken.load(std::memory_order_acquire) == 0) {
          break;
        }
      }

      buckets_[thread_id][insert % ABAOFFSET].store(
          new_item, std::memory_order_release);
      insert_[thread_id]->store(
          insert + 1 + ABAOFFSET, std::memory_order_release);
    };

    /////////////////////////////////////////////////////////////////
    //
    /////////////////////////////////////////////////////////////////
    bool try_remove_youngest(T *element, uint64_t *threshold) {
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
      // Indicates the index which contains the youngest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the youngest item found until now.
      uint64_t timestamp = 0;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      uint64_t old_remove_index = 0;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        Item* item;
        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local buffer.
        uint64_t tmp_remove_index = 
          insert_[tmp_buffer_index]->load();
        // We get the youngest element from that thread-local buffer.
        uint64_t tmp_buffer_array_index = 
          get_youngest_item(tmp_buffer_index, &item);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {
          empty = false;
          uint64_t item_timestamp = 
            item->timestamp.load(std::memory_order_acquire);
          if (timestamp < item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            buffer_array_index = tmp_buffer_array_index;
            old_remove_index = tmp_remove_index;
          } 
          // Check if we can remove the element immediately.
          if (*threshold <= timestamp) {
            uint64_t expected = 0;
            if (result->taken.compare_exchange_weak(
                    expected, 1, 
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
              // Try to adjust the remove pointer. It does not matter if this 
              // CAS fails.
              insert_[buffer_index]->compare_exchange_weak(
                  old_remove_index, buffer_array_index,
                  std::memory_order_acq_rel, std::memory_order_relaxed);
              *element = result->data.load(std::memory_order_acquire);
              return true;
            }
          }
        } else {
          // No element was found, work on the emptiness check.
          if (empty) {
            if (emptiness_check_pointers[tmp_buffer_index] 
                != tmp_buffer_array_index) {
              empty = false;
              emptiness_check_pointers[tmp_buffer_index] = 
                tmp_buffer_array_index;
            }
          }
        }
      }
      if (result != NULL) {
        // We found a youngest element which is not younger than the threshold.
        // We try to remove it.
        uint64_t expected = 0;
        if (result->taken.compare_exchange_weak(
                expected, 1, 
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
          // Try to adjust the remove pointer. It does not matter if this 
          // CAS fails.
          insert_[buffer_index]->compare_exchange_weak(
              old_remove_index, buffer_array_index,
              std::memory_order_acq_rel, std::memory_order_relaxed);
          *element = result->data.load(std::memory_order_acquire);
          return true;
        } else {
          // We were not able to remove the youngest element. However, in
          // later invocations we can remove any element younger than the
          // one we found in this iteration. Therefore the time stamp of
          // the element we found in this round will be the threshold for
          // the next round.
          *threshold = result->timestamp.load() + 1;
          *element = 0;
        }
      }

      *element = 0;
      return !empty;
      return false;
    }
};

//////////////////////////////////////////////////////////////////////
// A TSBuffer based on thread-local linked lists.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TLLinkedListBuffer : public TSBuffer<T> {
  private:
    typedef struct Item {
      Item* next;
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
      std::atomic<uint64_t> taken;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    // The thread-local queues, implemented as arrays of size BUFFERSIZE. 
    // At the moment buffer overflows are not considered.
    std::atomic<Item*> **buckets_;
    // The insert pointers of the thread-local queues.
    std::atomic<uint64_t>* *insert_;
    // The pointers for the emptiness check.
    Item** *emptiness_check_pointers_;

    void *get_aba_free_pointer(void *pointer) {
      uint64_t result = (uint64_t)pointer;
      result &= 0xfffffff8;
      return (void*)result;
    }

    void *add_next_aba(void *pointer, void *old, uint64_t increment) {
      uint64_t aba = (uint64_t)old;
      aba += increment;
      aba &= 0x7;
      uint64_t result = (uint64_t)pointer;
      result = (result & 0xfffffff8) | aba;
      return (void*)((result & 0xfffffff8) | aba);
    }

    // Returns the oldest not-taken item from the thread-local queue 
    // indicated by thread_id.
    Item* get_youngest_item(uint64_t thread_id) {

      Item* result = (Item*)get_aba_free_pointer(
        buckets_[thread_id]->load(std::memory_order_acquire));

      while (result->taken.load(std::memory_order_acquire) != 0 &&
          result->next != result) {
        result = result->next;
      }
      if (result == result->next) {
        return NULL;
      } else {
        return result;
      }
    }

  public:
    /////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////
    TLLinkedListBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      buckets_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {

        buckets_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch);
        new_item->timestamp.store(0, std::memory_order_release);
        new_item->data.store(0, std::memory_order_release);
        new_item->taken.store(1, std::memory_order_release);
        new_item->next = new_item;
        buckets_[i]->store(new_item, std::memory_order_release);

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
      new_item->timestamp.store(timestamp, std::memory_order_release);
      new_item->data.store(element, std::memory_order_release);
      new_item->taken.store(0, std::memory_order_release);

      Item* old_top = buckets_[thread_id]->load(std::memory_order_acquire);

      Item* top = (Item*)get_aba_free_pointer(old_top);
      while (top->next != top 
          && top->taken.load(std::memory_order_acquire)) {
        top = top->next;
      }

      new_item->next = top;
      buckets_[thread_id]->store(
          (Item*) add_next_aba(new_item, old_top, 1), 
          std::memory_order_release);
    };

    /////////////////////////////////////////////////////////////////
    // try_remove_youngest
    /////////////////////////////////////////////////////////////////
    bool try_remove_youngest(T *element, uint64_t *threshold) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      printf ("Thread %lu starts try\n", thread_id);
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the youngest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the youngest item found until now.
      uint64_t timestamp = 0;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_top = 0;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local buffer.
        Item* tmp_top = buckets_[tmp_buffer_index]->load();
        // We get the youngest element from that thread-local buffer.
        Item* item = get_youngest_item(tmp_buffer_index);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {
          empty = false;
          uint64_t item_timestamp = 
            item->timestamp.load(std::memory_order_acquire);
          if (timestamp < item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_top = tmp_top;
          } 
          // Check if we can remove the element immediately.
          if (*threshold <= timestamp) {
            uint64_t expected = 0;
            if (result->taken.compare_exchange_weak(
                    expected, 1, 
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
              // Try to adjust the remove pointer. It does not matter if 
              // this CAS fails.
//              buckets_[buffer_index]->compare_exchange_weak(
//                  old_top, (Item*)add_next_aba(result, old_top, 0), 
//                  std::memory_order_acq_rel, std::memory_order_relaxed);

              *element = result->data.load(std::memory_order_acquire);
              printf ("Thread %lu finishes try\n", thread_id);
              return true;
            }
          }
        } else {
          // No element was found, work on the emptiness check.
          if (empty) {
            if (emptiness_check_pointers[tmp_buffer_index] 
                != tmp_top) {
              empty = false;
              emptiness_check_pointers[tmp_buffer_index] = 
                tmp_top;
            }
          }
        }
      }
      if (result != NULL) {
        // We found a youngest element which is not younger than the threshold.
        // We try to remove it.
        uint64_t expected = 0;
        if (result->taken.compare_exchange_weak(
                expected, 1, 
                std::memory_order_acq_rel, std::memory_order_relaxed)) {
          // Try to adjust the remove pointer. It does not matter if this 
          // CAS fails.
//          buckets_[buffer_index]->compare_exchange_weak(
//              old_top, (Item*)add_next_aba(result, old_top, 0), 
//              std::memory_order_acq_rel, std::memory_order_relaxed);
          *element = result->data.load(std::memory_order_acquire);
              printf ("Thread %lu finishes try\n", thread_id);
          return true;
        } else {
          *threshold = result->timestamp.load() + 1;
          *element = (T)NULL;
        }
      }

      *element = (T)NULL;
      printf ("Thread %lu finishes try unsuccessfully", thread_id);
      return !empty;
    }
};

template<typename T>
class TSStack : public Stack<T> {
 private:
  TSBuffer<T> *buffer_;
  TimeStamp *timestamping_;
 public:
  explicit TSStack(TSBuffer<T> *buffer, TimeStamp *timestamping) 
    : buffer_(buffer), timestamping_(timestamping) {
  }
  bool push(T element);
  bool pop(T *element);
};

template<typename T>
bool TSStack<T>::push(T element) {
  uint64_t timestamp = timestamping_->get_timestamp();
  buffer_->insert_element(element, timestamp);
  return true;
}

template<typename T>
bool TSStack<T>::pop(T *element) {
  uint64_t threshold = timestamping_->get_timestamp();
  while (buffer_->try_remove_youngest(element, &threshold)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_Stack_H_
