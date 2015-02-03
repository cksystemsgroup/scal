// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SCAL_DATASTRUCTURES_LTS_QUEUE_H_
#define SCAL_DATASTRUCTURES_LTS_QUEUE_H_
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
class LTSQueue : Queue<T>{
  private:
    typedef struct Item {
      std::atomic<Item*> next;
      std::atomic<T> data;
      std::atomic<uint64_t> taken; 
      std::atomic<uint64_t> timestamp[2];
    } Item;

    typedef struct SPBuffer {
      std::atomic<Item*> *insert;
      std::atomic<Item*> *remove;
      std::atomic<SPBuffer*> next;
      int64_t index;
    } SPBuffer;

    uint64_t num_threads_;

    std::atomic<uint64_t> unlink_lock;
    // A map from thread IDs to spBuffers needed for insertion. For 
    // dynamic numbers of threads a hashmap could be used.
    std::atomic<SPBuffer*> *spBuffers_;
    std::atomic<SPBuffer*> entry_buffer_;
    AtomicCounterTimestamp *timestamping_;
    AtomicCounterTimestamp *dequeue_timestamping_;
    std::atomic<Item*> **insert_;
    std::atomic<Item*> **remove_;

#ifdef DTS_DEBUG
    uint64_t* *counter1_;
    uint64_t* *counter2_;
#endif

    inline void unlink_SPBuffer(SPBuffer* buffer) {

      if (buffer->index == -1 || spBuffers_[buffer->index] != NULL) {
        return;
      }

      uint64_t tmp = 0;
      if (!unlink_lock.compare_exchange_weak(tmp, 1)) {
        return;
      }

      Item* item = get_youngest_item(buffer);
      // Do not unlink a SP buffer which still contains an element.
      if (item != NULL) {
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
      new_item->taken.store(0);
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
    void initialize(uint64_t num_threads) {

      num_threads_ = num_threads;

      timestamping_ = static_cast<AtomicCounterTimestamp*>(
          scal::get<AtomicCounterTimestamp>(scal::kCachePrefetch * 4));

      timestamping_->initialize(0, num_threads);

      dequeue_timestamping_ = static_cast<AtomicCounterTimestamp*>(
          scal::get<AtomicCounterTimestamp>(scal::kCachePrefetch * 4));

      dequeue_timestamping_->initialize(0, num_threads);

      spBuffers_ = static_cast<std::atomic<SPBuffer*>*>(
          scal::ThreadLocalAllocator::Get().CallocAligned(num_threads_, sizeof(std::atomic<SPBuffer*>), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {
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
      new_item->taken.store(0);
      buffer->insert->store(new_item);
      buffer->remove->store(new_item);
      buffer->index = -1;
      entry_buffer_.store(buffer);

#ifdef DTS_DEBUG
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
//      avg1 = (double)0;

      double avg2 = sum2;
      avg2 /= (double)sum1;

      char buffer[255] = { 0 };
      uint32_t n = snprintf(buffer,
                            sizeof(buffer),
                            " ,\"c1\": %.2f ,\"c2\": %.2f",
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
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      // Create a new item.
      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      timestamping_->set_timestamp(new_item->timestamp);
      new_item->data.store(element);
      new_item->next.store(NULL);
      new_item->taken.store(0);

      SPBuffer *buffer = spBuffers_[thread_id].load();
      if (buffer == NULL) {
        buffer = register_thread(thread_id);
      }

      buffer->insert->load()->next.store(new_item);
      buffer->insert->store(new_item);
    };

    bool try_remove(SPBuffer* buffer, T *element, uint64_t *dequeue_timestamp) {
      Item* tmp_remove = buffer->remove->load();
      Item* tmp_insert = buffer->insert->load();

      if (tmp_remove == tmp_insert) {
        return false;
      }

      Item *prev = tmp_remove;
      Item *prev_next = prev->next.load();
      Item *current_item = prev_next;
      uint64_t timestamp[2];
      while (true) {
        if (current_item == NULL) {
          return false;
        }
        timestamping_->load_timestamp(timestamp, current_item->timestamp);
        
        if (dequeue_timestamp[0] == timestamp[0]) {
          Item* next = current_item->next;
          if (next != NULL) {
            prev->next.compare_exchange_weak(prev_next, next);
          }
          current_item->taken.store(1);
          *element = current_item->data.load();
          return true;
        } else if (timestamping_->is_later(timestamp, dequeue_timestamp)) {
          return false;
        } else if (current_item->taken.load() == 0) {
          prev = current_item;
          prev_next = prev->next.load();
          current_item = prev_next;
        } else {
          current_item = current_item->next.load();
        }
      }
    }

    inline bool enqueue(T element) {
      insert_element(element);
      return true;
    }

    inline bool dequeue(T *element) {
      uint64_t dequeue_timestamp[2];
      dequeue_timestamping_->set_timestamp_local(dequeue_timestamp);
      // We start iterating over the thread-local lists at a random index.
      SPBuffer* current_buffer;
      current_buffer = entry_buffer_.load()->next.load();
      // We iterate over all thead-local buffers
      while (true) {

        if (try_remove(current_buffer, element, dequeue_timestamp)) {
          return true;
        } else {
          current_buffer = current_buffer->next.load();
        }
      }
    }
};

#endif  // SCAL_DATASTRUCTURES_LTS_QUEUE_H_
