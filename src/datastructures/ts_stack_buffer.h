// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_STACK_BUFFER_H_
#define SCAL_DATASTRUCTURES_TS_STACK_BUFFER_H_

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "util/threadlocals.h"
#include "util/random.h"
#include "util/malloc.h"
#include "util/platform.h"

template<typename T, typename Timestamp>
class TSStackBuffer {
  private:
    typedef struct Item {
      std::atomic<Item*> next;
      std::atomic<uint64_t> taken;
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp[2];
    } Item;

    uint64_t num_threads_;

    std::atomic<Item*> **buckets_;
    Item** *emptiness_check_pointers_;
    Timestamp *timestamping_;
    uint64_t* *counter1_;
    uint64_t* *counter2_;

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

    // Returns the oldest not-taken item from the thread-local list 
    // indicated by thread_id.
    inline Item* get_youngest_item(uint64_t thread_id) {

      Item* result = (Item*)get_aba_free_pointer(
        buckets_[thread_id]->load(std::memory_order_acquire));

      while (true) {
        if (result->taken.load() == 0) {
          return result;
        }
        Item* next = result->next.load();
        if (next == result) {
          return NULL;
        }
        result = next;
      }
    }

  public:

    void initialize(uint64_t num_threads, Timestamp *timestamping) {

      num_threads_ = num_threads;
      timestamping_ = timestamping; 

      buckets_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        buckets_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        timestamping_->init_sentinel_atomic(new_item->timestamp);
        new_item->data.store(0);
        new_item->taken.store(1);
        new_item->next.store(new_item);
        buckets_[i]->store(new_item);

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }

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
    }

    inline void inc_counter1(uint64_t value) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      (*counter1_[thread_id]) += value;
    }
    
    inline void inc_counter2(uint64_t value) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      (*counter2_[thread_id]) += value;
    }
    
    char* ds_get_stats(void) {

      return NULL;
      // We do not print any DS stats at the moment.
      uint64_t sum1 = 0;
      uint64_t sum2 = 1;

      for (int i = 0; i < num_threads_; i++) {
        sum1 += *counter1_[i];
        sum2 += *counter2_[i];
      }

      char buffer[255] = { 0 };
      uint32_t n = snprintf(buffer,
                            sizeof(buffer),
                            ";c1: %lu ;c2: %lu",
                            sum1, sum2);
      if (n != strlen(buffer)) {
        fprintf(stderr, "%s: error creating stats string\n", __func__);
        abort();
      }
      char *newbuf = static_cast<char*>(calloc(
          strlen(buffer) + 1, sizeof(*newbuf)));
      return strncpy(newbuf, buffer, strlen(buffer));
    }

    inline std::atomic<uint64_t> *insert_right(T element) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      // Allocate a new item.
      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      timestamping_->init_top_atomic(new_item->timestamp);
      new_item->data.store(element);
      new_item->taken.store(0);

      Item* old_top = buckets_[thread_id]->load();

      // Find the topmost item in the thread-local list which has not been
      // removed logically yet.
      Item* top = (Item*)get_aba_free_pointer(old_top);
      while (top->next.load() != top 
          && top->taken.load()) {
        top = top->next.load();
      }

      // Add the new item to the thread-local list.
      new_item->next.store(top);
      buckets_[thread_id]->store(
          (Item*) add_next_aba(new_item, old_top, 1), 
          std::memory_order_release);

      // Return a pointer to the timestamp location of the item so that
      // a timestamp can be added.
      return new_item->timestamp;
    };

    inline std::atomic<uint64_t> *insert_left(T element) {
      // No explicit insert_left operation is provided, add the element
      // at the right side instead.
      return insert_right(element);
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_right
    /////////////////////////////////////////////////////////////////
    inline bool try_remove_right(T *element, uint64_t *invocation_time) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been found yet.
      Item *result = NULL;
      // Indicates the index of the buffer which contains the youngest item.
      uint64_t buffer_index = -1;
      // Memory on the stack frame where timestamps of items can be stored 
      // temporarily.
      uint64_t tmp_timestamp[2][2];
      // Index in the tmp_timestamp array which is not used at the moment.
      uint64_t tmp_index = 1;
      timestamping_->init_sentinel(tmp_timestamp[0]);
      // timestamp stores a pointer to the timestamp of the item with the
      // latest timestamp. 
      uint64_t *timestamp = tmp_timestamp[0];
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_top = NULL;

      // We start iterating over the thread-local lists at a random index.
      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {
        inc_counter1(1);

        // The index of the current buffer.
        uint64_t tmp_buffer_index = (start + i) % (num_threads_);
        // We get the remove/insert pointer of the current thread-local buffer.
        Item* tmp_top = buckets_[tmp_buffer_index]->load();
        // We get the youngest element from that thread-local buffer.
        Item* item = get_youngest_item(tmp_buffer_index);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {
          // We found an element, so the buffer is not empty.
          empty = false;
          uint64_t *item_timestamp;
          timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
          item_timestamp = tmp_timestamp[tmp_index];

          // Check if we can remove the element immediately.
          if (!timestamping_->is_later(invocation_time, item_timestamp)) {
            uint64_t expected = 0;
            // We try to set the taken flag and thereby logically remove the item.
            if (item->taken.compare_exchange_weak(
                    expected, 1, 
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
              // Try to adjust the remove pointer. It does not matter if 
              // this CAS fails.
              buckets_[tmp_buffer_index]->compare_exchange_weak(
                  tmp_top, (Item*)add_next_aba(item, tmp_top, 0), 
                  std::memory_order_acq_rel, std::memory_order_relaxed);

              // The item has been removed. 
              *element = item->data.load(std::memory_order_acquire);
              return true;
            } else {
              inc_counter2(1);
              // Elimination failed, we have to load a new element of that
              // buffer.
              tmp_top = buckets_[tmp_buffer_index]->load();
              item = get_youngest_item(tmp_buffer_index);
              if (item != NULL) {
                timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
                item_timestamp = tmp_timestamp[tmp_index];
              }
            }
          }
          if (item != NULL && timestamping_->is_later(item_timestamp, timestamp)) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            tmp_index ^=1;
            old_top = tmp_top;
           
          }
        } else {
          // No element was found, work on the emptiness check.
          if (emptiness_check_pointers[tmp_buffer_index] 
              != tmp_top) {
            empty = false;
            emptiness_check_pointers[tmp_buffer_index] = 
              tmp_top;
          }
        }
      }
      if (result != NULL) {
        // We found a youngest element which is not younger than the 
        // invocation time. We try to remove it.
        uint64_t expected = 0;
        if (result->taken.load() == 0) {
          if (result->taken.compare_exchange_weak(expected, 1)) {
            // Try to adjust the remove pointer. It does not matter if this 
            // CAS fails.
            buckets_[buffer_index]->compare_exchange_weak(
                old_top, (Item*)add_next_aba(result, old_top, 0));
            *element = result->data.load();
            return true;
          } else {
            inc_counter2(1);
          }
        }

        *element = (T)NULL;
      }

      *element = (T)NULL;
      return !empty;
    }

    inline bool try_remove_left(T *element, uint64_t *invocation_time) {
      // No explicit try_remove_left operation is provided, the element is
      // removed at the right side instead.
      return try_remove_right(element, invocation_time);
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_STACK_BUFFER_H_
