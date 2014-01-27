// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_DTS_QUEUE_H_
#define SCAL_DATASTRUCTURES_DTS_QUEUE_H_
#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/ts_timestamp.h"
#include "datastructures/queue.h"
#include "util/threadlocals.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

#define DTS_DEBUG

template<typename T>
class DTSQueue : Queue<T>{
  private:
    typedef struct Item {
      std::atomic<Item*> next;
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp[2];
    } Item;

    uint64_t num_threads_;
    AtomicCounterTimestamp *timestamping_;
    AtomicCounterTimestamp *dequeue_timestamping_;
    std::atomic<Item*> **insert_;
    std::atomic<Item*> **remove_;

#ifdef DTS_DEBUG
    uint64_t* *counter1_;
    uint64_t* *counter2_;
#endif

    // Helper function to remove the ABA counter from a pointer.
    inline void *get_aba_free_pointer(void *pointer) {
      uint64_t result = (uint64_t)pointer;
      result &= 0xfffffffffffffff8;
      return (void*)result;
    }

    // Helper function which retrieves the ABA counter of the pointer old
    // and sets this ABA counter + increment to the pointer pointer.
    inline void *add_next_aba(void *pointer, void *old, uint64_t increment) {
      uint64_t aba = (uint64_t)old;
      aba += increment;
      aba &= 0x7;
      uint64_t result = (uint64_t)pointer;
      result = (result & 0xfffffffffffffff8) | aba;
      return (void*)((result & 0xffffffffffffff8) | aba);
    }

  public:
    void initialize(uint64_t num_threads) {

      num_threads_ = num_threads;

      timestamping_ = static_cast<AtomicCounterTimestamp*>(
          scal::get<AtomicCounterTimestamp>(scal::kCachePrefetch * 4));

      timestamping_->initialize(0, num_threads);

      dequeue_timestamping_ = static_cast<AtomicCounterTimestamp*>(
          scal::get<AtomicCounterTimestamp>(scal::kCachePrefetch * 4));

      dequeue_timestamping_->initialize(0, num_threads);

      insert_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      remove_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        insert_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        remove_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        timestamping_->init_sentinel_atomic(new_item->timestamp);
        new_item->data.store(0);
        new_item->next.store(NULL);
        insert_[i]->store(new_item);
        remove_[i]->store(new_item);
      }

#ifdef DTS_DEBUG
      counter1_ = static_cast<uint64_t**>(
          scal::calloc_aligned(num_threads, sizeof(uint64_t*),
            scal::kCachePrefetch * 4));
      counter2_ = static_cast<uint64_t**>(
          scal::calloc_aligned(num_threads, sizeof(uint64_t*),
            scal::kCachePrefetch * 4));

      for (uint64_t i = 0; i < num_threads; i++) {
        counter1_[i] = scal::get<uint64_t>(scal::kCachePrefetch * 4);
        *(counter1_[i]) = 0;
        counter2_[i] = scal::get<uint64_t>(scal::kCachePrefetch * 4);
        *(counter2_[i]) = 0;
      }
#endif
    }

#ifdef DTS_DEBUG
    inline void inc_counter1(uint64_t value) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      (*counter1_[thread_id]) += value;
    }
    
    inline void inc_counter2(uint64_t value) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      (*counter2_[thread_id]) += value;
    }
#endif

    char* ds_get_stats(void) {
#ifdef DTS_DEBUG
      uint64_t sum1 = 0;
      uint64_t sum2 = 1;

      for (int i = 0; i < num_threads_; i++) {
        sum1 += *counter1_[i];
        sum2 += *counter2_[i];
      }

      if (sum1 == 0) {
        // Avoid division by zero.
        sum1 = 1;
      }

      double avg1 = sum1;
      avg1 = (double)0;

      double avg2 = sum2;
      avg2 /= (double)sum1;

      char buffer[255] = { 0 };
      uint32_t n = snprintf(buffer,
                            sizeof(buffer),
                            ";c1: %.2f ;c2: %.2f",
                            avg1, avg2);
      if (n != strlen(buffer)) {
        fprintf(stderr, "%s: error creating stats string\n", __func__);
        abort();
      }
      char *newbuf = static_cast<char*>(calloc(
          strlen(buffer) + 1, sizeof(*newbuf)));
      return strncpy(newbuf, buffer, strlen(buffer));
#else
      return NULL;
#endif
    }

    inline void insert_element(T element) {
#ifdef DTS_DEBUG
      inc_counter1(1);
#endif
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      // Create a new item.
      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      timestamping_->set_timestamp(new_item->timestamp);
      new_item->data.store(element);
      new_item->next.store(NULL);

      // Add the item to the thread-local list.
      Item* old_insert = insert_[thread_id]->load();
      old_insert->next.store(new_item);
      insert_[thread_id]->store(new_item);
    };

    bool try_remove_oldest(T *element, uint64_t *dequeue_timestamp) {
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index of the buffer which contains the oldest item.
      uint64_t buffer_index = -1;
      // Memory on the stack frame where timestamps of items can be stored
      // temporarily.
      uint64_t tmp_timestamp[2][2];
      // Index in the tmp_timestamp array where no timestamp is stored at the
      // moment.
      uint64_t tmp_index = 1;
      timestamping_->init_top(tmp_timestamp[0]);
      // Pointer to the earliest timestamp found until now.
      uint64_t *timestamp = tmp_timestamp[0];
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_remove = NULL;

      // We start iterating of the thread-local lists at a random index.
      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      uint64_t num_buffers = num_threads_;
      for (uint64_t i = 0; i < num_buffers; i++) {
#ifdef DTS_DEBUG
        inc_counter2(1);
#endif

        uint64_t tmp_buffer_index = (start + i) % (num_buffers);

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
          uint64_t *item_timestamp;
          timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
          item_timestamp = tmp_timestamp[tmp_index];

          // Check if we can remove the element immediately.
          if (!timestamping_->is_later(item_timestamp, dequeue_timestamp)) {
            uint64_t expected = 0;
            if ((remove_[tmp_buffer_index]->load() == tmp_remove) &&
                remove_[tmp_buffer_index]->compare_exchange_weak(
                    tmp_remove, (Item*)add_next_aba(item, tmp_remove, 1))) {

              // The item has been removed. 
              *element = item->data.load(std::memory_order_acquire);
              return true;
            } else {
              // Elimination failed, we have to load a new element of that
              // buffer.
              tmp_remove = remove_[tmp_buffer_index]->load();
              tmp_insert = insert_[tmp_buffer_index]->load();
              item =((Item*)get_aba_free_pointer(tmp_remove))->next.load();
              if (get_aba_free_pointer(tmp_remove) != tmp_insert) {
                timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
                item_timestamp = tmp_timestamp[tmp_index];
              }
            }
          }

          if (get_aba_free_pointer(tmp_remove) != tmp_insert) {
            if (timestamping_->is_later(timestamp, item_timestamp)) {
              // We found a new oldest element, so we remember it.
              result = item;
              buffer_index = tmp_buffer_index;
              timestamp = item_timestamp;
              tmp_index ^=1;
              old_remove = tmp_remove;
            } 
          }
        }
      }
      if (result != NULL) {
        if (remove_[buffer_index]->load() == old_remove) {
          if (remove_[buffer_index]->compare_exchange_weak(
                old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
            *element = result->data.load();
            return true;
          }
        }
      }
      *element = (T)NULL;
      return true;
    }

    inline bool enqueue(T element) {
      insert_element(element);
      return true;
    }

    inline bool dequeue(T *element) {
      uint64_t dequeue_timestamp[2];
        dequeue_timestamping_->set_timestamp_local(dequeue_timestamp);
      while (try_remove_oldest(element, dequeue_timestamp)) {

        if (*element != (T)NULL) {
          return true;
        }
      }
      // This is unreachable code, because this queue blocks when no 
      // element can be found, i.e. there does not exist an emptiness
      // check.
      return false;
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_
