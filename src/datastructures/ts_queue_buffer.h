// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_
#define SCAL_DATASTRUCTURES_TS_QUEUE_BUFFER_H_

#define __STDC_FORMAT_MACROS 1  // we want PRIu64 and friends
#define __STDC_LIMIT_MACROS
#include <inttypes.h>
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

    typedef struct SPBuffer {
      std::atomic<Item*> *insert;
      std::atomic<Item*> *remove;
      std::atomic<SPBuffer*> next;
      int64_t index;
    } SPBuffer;

    std::atomic<uint64_t> unlink_lock;
    // A map from thread IDs to spBuffers needed for insertion. For 
    // dynamic numbers of threads a hashmap could be used.
    std::atomic<SPBuffer*> *spBuffers_;
    std::atomic<SPBuffer*> entry_buffer_;

    uint64_t num_threads_;
    TimeStamp *timestamping_;
    Item** *emptiness_check_pointers_;
    uint64_t* *counter1_;
    uint64_t* *counter2_;

    inline void unlink_SPBuffer(SPBuffer* buffer) {

      if (buffer->index == -1 || spBuffers_[buffer->index] != NULL) {
        return;
      }

      uint64_t tmp = 0;
      if (!unlink_lock.compare_exchange_weak(tmp, 1)) {
        return;
      }

      Item* tmp_remove = buffer->remove->load();
      Item* tmp_insert = buffer->insert->load();

      if (get_aba_free_pointer(tmp_remove) != tmp_insert) {
        unlink_lock.store(0);
        return;
      }
      // Find the SP buffer which stores the buffer to be removed in its 
      // next pointer
      SPBuffer *prev = entry_buffer_;
      while (prev->next != entry_buffer_ && prev->next != buffer) {
        prev = prev->next;
      }

      SPBuffer *next =prev->next;

      if (next == entry_buffer_) {
        // The buffer already got removed.
        unlink_lock.store(0);
        return;
      }

      // Unlink the buffer.
      prev->next.compare_exchange_weak(next, buffer->next);
      unlink_lock.store(0);
    }

    inline SPBuffer *register_thread(uint64_t thread_id) {

      SPBuffer* buffer = scal::tlget_aligned<SPBuffer>(scal::kCachePrefetch);
      spBuffers_[thread_id].store(buffer);
      buffer->next.store(buffer);

      buffer->insert = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      buffer->remove = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      // Add a sentinal node.
      Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
      timestamping_->init_sentinel_atomic(new_item->timestamp);
      new_item->data.store(0);
      new_item->next.store(NULL);
      buffer->insert->store(new_item);
      buffer->remove->store(new_item);
      buffer->index = thread_id;

      SPBuffer* entry = entry_buffer_.load();
      SPBuffer* next = entry->next.load();
      while (true) {
        buffer->next.store(next);
        if (entry->next.compare_exchange_weak(next, buffer)) {
          return buffer;
        }
      }
    }

    inline void unregister_thread(uint64_t thread_id) {
      spBuffers_[thread_id].store(NULL);
    }


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

      spBuffers_ = static_cast<std::atomic<SPBuffer*>*>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(std::atomic<SPBuffer*>), 
            scal::kCachePrefetch * 4));

      for (uint64_t i = 0; i < num_threads_; i++) {
         spBuffers_[i].store(NULL); 
      }
      
      // Create the entry buffer.
      SPBuffer* buffer = scal::tlget_aligned<SPBuffer>(scal::kCachePrefetch);
      buffer->next.store(buffer);

      buffer->insert = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      buffer->remove = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      // Add a sentinal node.
      Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
      timestamping_->init_sentinel_atomic(new_item->timestamp);
      new_item->data.store(0);
      new_item->next.store(NULL);
      buffer->insert->store(new_item);
      buffer->remove->store(new_item);
      buffer->index = -1;
      entry_buffer_.store(buffer);

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (uint64_t i = 0; i < num_threads_; i++) {

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }
      counter1_ = static_cast<uint64_t**>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads, sizeof(uint64_t*),
            scal::kCachePrefetch * 4));
      counter2_ = static_cast<uint64_t**>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads, sizeof(uint64_t*),
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

      uint64_t sum1 = 0;
      uint64_t sum2 = 1;

      for (uint64_t i = 0; i < num_threads_; i++) {
        sum1 += *counter1_[i];
        sum2 += *counter2_[i];
      }

      char buffer[255] = { 0 };
      uint32_t n = snprintf(buffer,
                            sizeof(buffer),
                            " ,\"c1\": %lu ,\"c2\": %lu",
                            sum1, sum2);
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
      SPBuffer *buffer = spBuffers_[thread_id].load();
      if (buffer == NULL) {
        buffer = register_thread(thread_id);
      }

      buffer->insert->load()->next.store(new_item);
      buffer->insert->store(new_item);

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
      inc_counter2(1);
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
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
//       uint64_t start = hwrand() % num_threads_;
//       // We iterate over all thead-local buffers
// //      uint64_t num_buffers = (num_threads_ / 2) + 1;
//       uint64_t num_buffers = num_threads_;
 
      // We start iterating over the thread-local lists at a random index.
      uint64_t start = hwrand() % num_threads_;
      SPBuffer* current_buffer;
      SPBuffer* youngest_buffer;
      current_buffer = entry_buffer_.load();
      uint64_t entry_counter = 0;
      // Iterate to a random start buffer.
      for (uint64_t i = 0; i < start; i++) {
        current_buffer = current_buffer->next.load();
      }
      SPBuffer* start_buffer = current_buffer;
      // We iterate over all thead-local buffers
      while (true) {      
        // If we visit the entry buffer twice, then we know that we have
        // visited all SP buffers.
        if (current_buffer->index == -1) {
          entry_counter++;
        } 
        if (entry_counter >= 2) {
          break;
        }
        current_buffer = current_buffer->next.load();        

#ifdef DTS_DEBUG
        inc_counter2(1);
#endif

        // We get the remove/insert pointer of the current thread-local 
        // buffer.
        Item* tmp_remove = current_buffer->remove->load();
        Item* tmp_insert = current_buffer->insert->load();
        Item* item = NULL;
    //  for (uint64_t i = 0; i < num_buffers; i++) {

//        uint64_t tmp_buffer_index = (start + i) % num_buffers;
//        uint64_t tmp_buffer_index = (start + i);
//        if (tmp_buffer_index >= num_threads_) {
//          tmp_buffer_index -= num_threads_;
//        }

        // We get the remove/insert pointer of the current thread-local 
        // buffer.
//         Item* tmp_remove = remove_[tmp_buffer_index]->load();
//         Item* tmp_insert = insert_[tmp_buffer_index]->load();
//         Item* item = 
//           ((Item*)get_aba_free_pointer(tmp_remove))->next.load();
        // We get the oldest element from that thread-local buffer.
        // If we found an element, we compare it to the oldest element 
        // we have found until now.
        if (get_aba_free_pointer(tmp_remove) != tmp_insert) {
          empty = false;

          item = ((Item*)get_aba_free_pointer(tmp_remove))->next.load();
          uint64_t *item_timestamp;
          timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
          item_timestamp = tmp_timestamp[tmp_index];

          if (timestamping_->is_later(timestamp, item_timestamp)) {
            // We found a new oldest element, so we remember it.
            result = item;
//            buffer_index = tmp_buffer_index;
            youngest_buffer = current_buffer;
            timestamp = item_timestamp;
            tmp_index ^=1;
            old_remove = tmp_remove;
          } 
        } else {
          // No element was found, work on the emptiness check.
          if (current_buffer->index != -1 
            && emptiness_check_pointers[current_buffer->index] != tmp_remove) {
            empty = false;
            emptiness_check_pointers[current_buffer->index] = 
              tmp_remove;
          }
          unlink_SPBuffer(current_buffer);
        }
        if (current_buffer == start_buffer) {
          break;
        }
      }
      if (result != NULL) {
        if (!timestamping_->is_later(timestamp, start_time)) {
          if (youngest_buffer->remove->load() == old_remove) {
            if (youngest_buffer->remove->compare_exchange_weak(
                  old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
              *element = result->data.load();
              return true;
            }
          }
//          if (remove_[buffer_index]->load() == old_remove) {
//            if (remove_[buffer_index]->compare_exchange_weak(
//                  old_remove, (Item*)add_next_aba(result, old_remove, 1))) {
//              *element = result->data.load();
//              return true;
//            }
//          }
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
