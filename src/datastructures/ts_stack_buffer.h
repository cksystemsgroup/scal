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
#include "util/scal-time.h"

template<typename T, typename Timestamp>
class TSStackBuffer {
  private:
    // The item in a SP buffer.
    typedef struct Item {
      // A pointer to the next item in the SP buffer.
      std::atomic<Item*> next;
      // A flag indicating that the item was already removed.
      std::atomic<uint64_t> taken;
      // The actual element.
      std::atomic<T> data;
      // The timestamp of the element.
      std::atomic<uint64_t> timestamp[2];
    } Item;

    // The SP buffer.
    typedef struct SPBuffer {
      // A pointer to the top item in the SP buffer.
      std::atomic<Item*> *list;
      // All SP buffers are stored in a cyclic list. This is the next pointer
      // in this list.
      std::atomic<SPBuffer*> next;
      // The thread_id of the owner of this SP buffer. This index is used as
      // a hash value.
      int64_t index;
    } SPBuffer;

    uint64_t num_threads_;

    // Adding and removing SP buffers is protected by this lock.
    std::atomic<uint64_t> unlink_lock;
    // A map from thread IDs to SP buffers needed for insertion. We know the 
    // maximum number of threads and therefore use an array to implement the 
    // map. Hashmaps could be used otherwise.
    std::atomic<SPBuffer*> *spBuffers_;
    SPBuffer *prealloc_buffers_;
    // The entry point into the cyclic list of SP buffers.
    std::atomic<SPBuffer*> entry_buffer_;
    // A map from thread IDs to SP buffers for each thread needed for the 
    // emptiness-check. We know the maximum number of threads and therefore
    // use an array to implement the map. Hashmaps could be used otherwise.
    Item** *emptiness_check_pointers_;
    // The timestamping algorithm.
    Timestamp *timestamping_;
    // Thread-local counters for debugging.
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

    // Returns the youngest not-taken item from the given SP buffer.
    inline Item* get_youngest_item(SPBuffer *buffer, Item* *old_top) {

      *old_top = buffer->list->load();
      Item* result = (Item*)get_aba_free_pointer(*old_top);
      while (true) {
        if (result->taken.load() == 0) {
          return result;
        }
        Item* next = result->next.load();
        if (next == result) {
          // If the SP buffer is empty, we try to unlink it.
          unlink_SPBuffer(buffer);
          return NULL;
        }
        result = next;
      }
    }

    // A method to unlink SP buffer for the case when a thread terminates
    // and its SP buffer becomes empty.
    inline void unlink_SPBuffer(SPBuffer* buffer) {

      if (buffer->index == -1 || spBuffers_[buffer->index] != NULL) {
        return;
      }

      uint64_t tmp = 0;
      if (!unlink_lock.compare_exchange_weak(tmp, 1)) {
        return;
      }

      Item *old_top;
      Item* item = get_youngest_item(buffer, &old_top);
      if (item != NULL) {
        unlink_lock.store(0);
        return;
      }
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

    // Create a new SP buffer for the thread with the given thread ID.
    inline SPBuffer *register_thread(uint64_t thread_id) {

      SPBuffer* buffer = &prealloc_buffers_[thread_id];
//      SPBuffer* buffer = scal::tlget_aligned<SPBuffer>(scal::kCachePrefetch);
      spBuffers_[thread_id].store(buffer);
      buffer->next.store(buffer);

      buffer->list = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      // Add a sentinal node.
      Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
      timestamping_->init_sentinel_atomic(new_item->timestamp);
      new_item->data.store(0);
      new_item->taken.store(1);
      new_item->next.store(new_item);
      buffer->list->store(new_item);
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

    // Unregister the thread. The SP buffer will be unlinked when it becomes
    // empty.
    inline void unregister_thread(uint64_t thread_id) {
      spBuffers_[thread_id].store(NULL);
    }

  public:

    void initialize(uint64_t num_threads, Timestamp *timestamping) {

      unlink_lock.store(0);
      num_threads_ = num_threads;
      timestamping_ = timestamping; 

      spBuffers_ = static_cast<std::atomic<SPBuffer*>*>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(std::atomic<SPBuffer*>), 
            scal::kCachePrefetch * 4));

      emptiness_check_pointers_ = static_cast<Item***>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

       prealloc_buffers_ = static_cast<SPBuffer*> (
            scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(SPBuffer), 
              scal::kCachePrefetch * 4));

       for (uint64_t i = 0; i < num_threads_; i++) {
         spBuffers_[i].store(NULL); 

        emptiness_check_pointers_[i] = static_cast<Item**> (
            scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }

      // Create the entry buffer.
      SPBuffer* buffer = &prealloc_buffers_[0];
//      SPBuffer* buffer = scal::tlget_aligned<SPBuffer>(scal::kCachePrefetch);
      buffer->next.store(buffer);

      buffer->list = static_cast<std::atomic<Item*>*>(
          scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

      // Add a sentinal node.
      Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
      timestamping_->init_sentinel_atomic(new_item->timestamp);
      new_item->data.store(0);
      new_item->taken.store(1);
      new_item->next.store(new_item);
      buffer->list->store(new_item);
      buffer->index = -1;
      entry_buffer_.store(buffer);

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

    // Increases the first debug counter.
    inline void inc_counter1(uint64_t value) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      (*counter1_[thread_id]) += value;
    }
    
    // Increases the second debug counter.
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

    inline std::atomic<uint64_t> *insert_right(T element) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      // Allocate a new item.
      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      timestamping_->init_top_atomic(new_item->timestamp);
      new_item->data.store(element);
      new_item->taken.store(0);

      SPBuffer *buffer = spBuffers_[thread_id].load();
      if (buffer == NULL) {
        buffer = register_thread(thread_id);
      }

      Item* old_top = buffer->list->load();

      // Find the topmost item in the thread-local list which has not been
      // removed logically yet.
      Item* top = (Item*)get_aba_free_pointer(old_top);

      // Add the new item to the thread-local list.
      Item *next = top;
      new_item->next.store(top);

      buffer->list->store((Item*) add_next_aba(new_item, old_top, 1));

      while (top->next.load() != top 
          && top->taken.load()) {
        top = top->next.load();
      }
      // Add the new item to the thread-local list.
      if (next != top) {
        new_item->next.store(top);
      }
      // Return a pointer to the timestamp location of the item so that
      // a timestamp can be added.
      return new_item->timestamp;
    };

    inline std::atomic<uint64_t> *insert_left(T element) {
      // No explicit insert_left operation is provided, add the element
      // at the right side instead.
      return insert_right(element);
    }

    // A short delay in the loop of try_remove to reduce the pressure on the 
    // memory bus.
    inline void delay() {
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");

          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
          __asm__ __volatile__("nop");
    }

    inline bool remove (Item *item, SPBuffer *buffer, Item *top) {
      uint64_t expected = 0;
      if (item->taken.load() == 0 && item->taken.compare_exchange_weak(expected, 1)) {
        // Try to adjust the remove pointer. It does not matter if 
        // this CAS fails.
        Item *old_top = (Item*)get_aba_free_pointer(top);
        buffer->list->compare_exchange_weak(
            top, (Item*)add_next_aba(item, top, 0));

        // Unlinking.
        if (old_top != item) {
          old_top->next.store(item);
        }

         Item *next = item->next.load();
         Item *old_next = next;
         while (next->next.load() != next && next->taken.load()) {
           next = next->next.load();
         }
         if (next != old_next) {
           item->next.store(next);
         }
         return true;
      }
      return false;
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_right
    /////////////////////////////////////////////////////////////////
    inline bool try_remove_right(T *element, uint64_t *invocation_time) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_pointers = 
        emptiness_check_pointers_[thread_id];
      // Initialize the result pointer to NULL, which means that no 
      // element has been found yet.
      Item *result = NULL;
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
      uint64_t start = pseudorand() % num_threads_;
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

        if (current_buffer->index == -1) {
          entry_counter++;
        }
        // The start buffer may have been removed during the iteration, thus
        // we terminate the loop also when the entry buffer is visited twice.
        // The entry buffer cannot be removed.
        if (entry_counter >= 2) {
          break;
        }
        current_buffer = current_buffer->next.load();        
        Item* tmp_top;
        // We get the youngest element from that thread-local buffer.
        Item* item = get_youngest_item(current_buffer, &tmp_top);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {

          uint64_t *item_timestamp;
          timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
          item_timestamp = tmp_timestamp[tmp_index];

          delay();
          // Check if we can remove the element immediately.
          if (!timestamping_->is_later(invocation_time, item_timestamp)) {
            // We try to set the taken flag and thereby logically remove the item.
            if (remove(item, current_buffer, tmp_top)) {
      inc_counter2(1);
              // The item has been removed. 
              *element = item->data.load();
              return true;
            } else {
              // Elimination failed, we have to load a new element of that
              // buffer.
              item = get_youngest_item(current_buffer, &tmp_top);
              if (item != NULL) {
                timestamping_->load_timestamp(tmp_timestamp[tmp_index], item->timestamp);
                item_timestamp = tmp_timestamp[tmp_index];
              }
            }
          }
          if (item != NULL && timestamping_->is_later(item_timestamp, timestamp)) {
            // We found a new youngest element, so we remember it.
            result = item;
            youngest_buffer = current_buffer;
            timestamp = item_timestamp;
            tmp_index ^=1;
            old_top = tmp_top;
           
          }
        } else {
          // Emptiness check: no element for, record the top poiner.
          if (current_buffer->index != -1) {
            emptiness_check_pointers[current_buffer->index] = tmp_top;
          }
        }
        // We have seen all SP buffers, we can terminate the loop.
        if (current_buffer == start_buffer) {
          break;
        }
      }


      bool empty = false;
      if (result != NULL) {
        // We found a youngest element which is not younger than the 
        // invocation time. We try to remove it.
        if (remove(result, youngest_buffer, old_top)) {
          *element = result->data.load();
          return true;
        }

        *element = (T)NULL;
      } else {
        // Emptiness check.
        empty = true;
        start_buffer = current_buffer;
        entry_counter = 0;
        // We iterate over all thead-local buffers
        while (true) {      
          
          current_buffer = current_buffer->next.load();        

          if (current_buffer->index == -1) {
            entry_counter++;
            // The start buffer may have been removed during the iteration, thus
            // we terminate the loop also when the entry buffer is visited twice.
            // The entry buffer cannot be removed.
            if (entry_counter >= 2) {
              break;
            }
            continue;
          }
     
          if (current_buffer->list->load() != 
              emptiness_check_pointers[current_buffer->index]) {
            
            empty = false;
            break;
          }

          if (current_buffer == start_buffer) {
            break;
          }
        }
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
