// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_
#define SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/ts_timestamp.h"
#include "util/threadlocals.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

template<typename T, typename TimeStamp>
class TSQueueBuffer {
  private:
    typedef struct Item {
      std::atomic<Item*> next;
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp[2];
    } Item;

    uint64_t num_threads_;
    TimeStamp *timestamping_;
    std::atomic<Item*> **insert_;
    std::atomic<Item*> **remove_;
    Item** *emptiness_check_pointers_;

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
    void initialize(uint64_t num_threads, TimeStamp *timestamping) {

      num_threads_ = num_threads;
      timestamping_ = timestamping;

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
        timestamping_->init_sentinel_atomic(new_item->timestamp);
        new_item->data.store(0);
        new_item->next.store(NULL);
        insert_[i]->store(new_item);
        remove_[i]->store(new_item);

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }
    }

    char* ds_get_stats(void) {

      return NULL;
      // We do not print any DS stats at the moment.
      uint64_t sum1 = 0;
      uint64_t sum2 = 1;

      for (int i = 0; i < num_threads_; i++) {
//        sum1 += *counter1_[i];
//        sum2 += *counter2_[i];
      }

      double avg1 = sum1;
      avg1 /= (double)40000000.0;

      double avg2 = sum2;
      avg2 /= (double)40000000.0;

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
    }

    inline std::atomic<uint64_t> *insert_left(T element) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      // Create a new item.
      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      timestamping_->init_top_atomic(new_item->timestamp);
      new_item->data.store(element);
      new_item->next.store(NULL);

      // Add the item to the thread-local list.
      Item* old_insert = insert_[thread_id]->load();
      old_insert->next.store(new_item);
      insert_[thread_id]->store(new_item);

      //Return a pointer to the timestamp location of the item so that
      // a timestamp can be assigned.
      return new_item->timestamp;
    };

    inline std::atomic<uint64_t> *insert_right(T element) {
      // No explicit insert_right operation is provided, add the element
      // at the left side instead.
      return insert_left(element);
    }

    bool try_remove_right(T *element, uint64_t *invocation_time) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
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

      // Read the start time of the iteration. Items which were timestamped
      // after the start time do not get removed.
      uint64_t start_time[2];
      timestamping_->read_time(start_time);
      // We start iterating of the thread-local lists at a random index.
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
          empty = false;
          
          uint64_t *item_timestamp;
          timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
          item_timestamp = tmp_timestamp[tmp_index];

          if (timestamping_->is_later(timestamp, item_timestamp)) {
            // We found a new oldest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            tmp_index ^=1;
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
        if (!timestamping_->is_later(timestamp, start_time)) {
          if (remove_[buffer_index]->load() == old_remove) {
            if (remove_[buffer_index]->compare_exchange_weak(
                  old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
              *element = result->data.load();
              return true;
            }
          }
        }
      }
      *element = (T)NULL;
      return !empty;
    }

    bool try_remove_left(T *element, uint64_t *invocation_time) {
      // No explicit try_remove_left operation is provided, the element
      // is removed at the right side instead.
      return try_remove_right(element, invocation_time);
    }
};

#endif  // SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_
