#ifndef SCAL_DATASTRUCTURES_TS_DEQUE_H_
#define SCAL_DATASTRUCTURES_TS_DEQUE_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "datastructures/deque.h"
#include "datastructures/ts_timestamp.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

//////////////////////////////////////////////////////////////////////
// The base TSDequeBuffer class.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TSDequeBuffer {
  public:
    virtual void insert_left(T element, uint64_t timestamp) = 0;
    virtual void insert_right(T element, uint64_t timestamp) = 0;
    virtual bool try_remove_left(T *element, int64_t *threshold) = 0;
    virtual bool try_remove_right(T *element, int64_t *threshold) = 0;
};

//////////////////////////////////////////////////////////////////////
// A TSDequeBuffer based on thread-local linked lists.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TLLinkedListDequeBuffer : public TSDequeBuffer<T> {
  private:
    typedef struct Item {
      std::atomic<Item*> left;
      std::atomic<Item*> right;
      std::atomic<uint64_t> taken;
      std::atomic<T> data;
      std::atomic<int64_t> timestamp;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    std::atomic<Item*> **left_;
    std::atomic<Item*> **right_;
    // The pointers for the emptiness check.
    Item** *emptiness_check_left_;
    Item** *emptiness_check_right_;

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

    // Returns the oldest not-taken item from the thread-local queue 
    // indicated by thread_id.
    Item* get_left_item(uint64_t thread_id) {

      Item* old_right = right_[thread_id]->load(std::memory_order_seq_cst);
      Item* right = (Item*)get_aba_free_pointer(
          old_right);

      int64_t threshold = right->timestamp.load();

      Item* result = (Item*)get_aba_free_pointer(
        left_[thread_id]->load(std::memory_order_seq_cst));

      while (result->taken.load(std::memory_order_seq_cst) != 0 &&
          result->right.load() != result) {
        result = result->right.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the problem
      // that we may find an element inserted at the right side which is older
      // than an element which was inserted on the left side in the meantime.
      if (result->taken.load() != 0 ||
         (result->timestamp.load() > threshold &&
          right_[thread_id]->load() != old_right)) {
        return NULL;
      }
      return result;
    }

    Item* get_right_item(uint64_t thread_id) {

      Item* left = (Item*)get_aba_free_pointer(
          left_[thread_id]->load(std::memory_order_seq_cst));

      int64_t threshold = left->timestamp.load();

      Item* result = (Item*)get_aba_free_pointer(
        right_[thread_id]->load(std::memory_order_seq_cst));

      while (result->taken.load(std::memory_order_seq_cst) != 0 &&
          result->left.load() != result) {
        result = result->left.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the problem
      // that we may find an element inserted at the left side which is older
      // than an element which was inserted on the right side in the meantime.
      if (result->taken.load() != 0 || result->timestamp.load() < threshold) {
        return NULL;
      } else {
        return result;
      }
    }

  public:
    /////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////
    TLLinkedListDequeBuffer(uint64_t num_threads) : num_threads_(num_threads) {
      left_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      right_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 2));

      emptiness_check_left_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 2));

      emptiness_check_right_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 2));

      for (int i = 0; i < num_threads_; i++) {

        left_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch));

        right_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch);
        new_item->timestamp.store(0, std::memory_order_seq_cst);
        new_item->data.store(0, std::memory_order_seq_cst);
        new_item->taken.store(1, std::memory_order_seq_cst);
        new_item->left.store(new_item);
        new_item->right.store(new_item);
        left_[i]->store(new_item, std::memory_order_seq_cst);
        right_[i]->store(new_item, std::memory_order_seq_cst);

        emptiness_check_left_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 2));

        emptiness_check_right_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 2));
      }
    }

    /////////////////////////////////////////////////////////////////
    // insert_left
    /////////////////////////////////////////////////////////////////
    void insert_left(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      // Switch the sign of the time stamp of elements inserted at the left side.
      new_item->timestamp.store(((int64_t)timestamp) * (-1),
          std::memory_order_seq_cst);
      new_item->data.store(element, std::memory_order_seq_cst);
      new_item->taken.store(0, std::memory_order_seq_cst);
      new_item->left.store(new_item, std::memory_order_seq_cst);

      Item* old_left = left_[thread_id]->load(std::memory_order_seq_cst);

      Item* left = (Item*)get_aba_free_pointer(old_left);
      while (left->right.load() != left 
          && left->taken.load(std::memory_order_seq_cst)) {
        left = left->right.load();
      }

      if (left->right.load() == left) {
        // The buffer is empty. We have to increase the aba counter of the right
        // pointer too to guarantee that a pending right-pointer update of a
        // remove operation does not make the left and the right pointer point
        // to different lists.
        Item* old_right = right_[thread_id]->load(std::memory_order_seq_cst);
        right_[thread_id]->store( (Item*) add_next_aba(left, old_right, 1),
            std::memory_order_seq_cst); }

      new_item->right.store(left);
      left->left.store(new_item);
      left_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_left, 1),
        std::memory_order_seq_cst);
    }

    /////////////////////////////////////////////////////////////////
    // insert_right
    /////////////////////////////////////////////////////////////////
    void insert_right(T element, uint64_t timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->timestamp.store(timestamp, std::memory_order_seq_cst);
      new_item->data.store(element, std::memory_order_seq_cst);
      new_item->taken.store(0, std::memory_order_seq_cst);
      new_item->right.store(new_item, std::memory_order_seq_cst);

      Item* old_right = right_[thread_id]->load(std::memory_order_seq_cst);

      Item* right = (Item*)get_aba_free_pointer(old_right);
      while (right->left.load() != right 
          && right->taken.load(std::memory_order_seq_cst)) {
        right = right->left.load();
      }

      if (right->left.load() == right) {
        // The buffer is empty. We have to increase the aba counter of the left
        // pointer too to guarantee that a pending left-pointer update of a
        // remove operation does not make the left and the right pointer point
        // to different lists.
        Item* old_left = left_[thread_id]->load(std::memory_order_seq_cst);
        left_[thread_id]->store( (Item*) add_next_aba(right, old_left, 1),
            std::memory_order_seq_cst); }

      new_item->left.store(right);
      right->right.store(new_item);
      right_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_right, 1),
        std::memory_order_seq_cst);
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_left
    /////////////////////////////////////////////////////////////////
    bool try_remove_left(T *element, int64_t *threshold) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_left = 
        emptiness_check_left_[thread_id];
      Item* *emptiness_check_right = 
        emptiness_check_right_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the youngest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the left-most item found until now.
      int64_t timestamp = INT64_MAX;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_left = NULL;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local buffer.
        Item* tmp_left = left_[tmp_buffer_index]->load();
        // We get the youngest element from that thread-local buffer.
        Item* item = get_left_item(tmp_buffer_index);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {
          empty = false;
          int64_t item_timestamp = 
            item->timestamp.load(std::memory_order_seq_cst);

          if (timestamp > item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_left = tmp_left;
          } 
          // Check if we can remove the element immediately.
          if (*threshold > timestamp) {
            uint64_t expected = 0;
            if (result->taken.compare_exchange_weak(
                    expected, 1, 
                    std::memory_order_seq_cst, std::memory_order_relaxed)) {
              // Try to adjust the remove pointer. It does not matter if 
              // this CAS fails.
              left_[buffer_index]->compare_exchange_weak(
                  old_left, (Item*)add_next_aba(result, old_left, 0), 
                  std::memory_order_seq_cst, std::memory_order_relaxed);

              *element = result->data.load(std::memory_order_seq_cst);
              return true;
            }
          }
        } else {
          // No element was found, work on the emptiness check.
          if (emptiness_check_left[tmp_buffer_index] 
              != tmp_left) {
            empty = false;
            emptiness_check_left[tmp_buffer_index] = 
              tmp_left;
          }
          Item* tmp_right = right_[tmp_buffer_index]->load();
          if (emptiness_check_right[tmp_buffer_index] 
              != tmp_right) {
            empty = false;
            emptiness_check_right[tmp_buffer_index] = 
              tmp_right;
          }
        }
      }
      if (result != NULL) {
        if (timestamp == *threshold) {
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
          if (result->taken.compare_exchange_weak(
                  expected, 1, 
                  std::memory_order_seq_cst, std::memory_order_relaxed)) {
            // Try to adjust the remove pointer. It does not matter if this 
            // CAS fails.
            left_[buffer_index]->compare_exchange_weak(
                old_left, (Item*)add_next_aba(result, old_left, 0), 
                std::memory_order_seq_cst, std::memory_order_relaxed);
            *element = result->data.load(std::memory_order_seq_cst);
            return true;
          }
        }
        *threshold = result->timestamp.load();
      }

      *element = (T)NULL;
      return !empty;
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_right
    ////////////////////////////////////////////////////////////////
    bool try_remove_right(T *element, int64_t *threshold) {
      // Initialize the data needed for the emptiness check.
      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      Item* *emptiness_check_left = 
        emptiness_check_left_[thread_id];
      Item* *emptiness_check_right = 
        emptiness_check_right_[thread_id];
      bool empty = true;
      // Initialize the result pointer to NULL, which means that no 
      // element has been removed.
      Item *result = NULL;
      // Indicates the index which contains the youngest item.
      uint64_t buffer_index = -1;
      // Stores the time stamp of the right-most item found until now.
      int64_t timestamp = INT64_MIN;
      // Stores the value of the remove pointer of a thead-local buffer 
      // before the buffer is actually accessed.
      Item* old_right = NULL;

      uint64_t start = hwrand();
      // We iterate over all thead-local buffers
      for (uint64_t i = 0; i < num_threads_; i++) {

        uint64_t tmp_buffer_index = (start + i) % num_threads_;
        // We get the remove/insert pointer of the current thread-local buffer.
        Item* tmp_right = right_[tmp_buffer_index]->load();
        // We get the youngest element from that thread-local buffer.
        Item* item = get_right_item(tmp_buffer_index);
        // If we found an element, we compare it to the youngest element 
        // we have found until now.
        if (item != NULL) {
          empty = false;
          int64_t item_timestamp = 
            item->timestamp.load(std::memory_order_seq_cst);
          if (timestamp < item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_right = tmp_right;
          }
          // Check if we can remove the element immediately.
          if (*threshold < timestamp) {
            uint64_t expected = 0;
            if (result->taken.compare_exchange_weak(
                  expected, 1, 
                  std::memory_order_seq_cst, std::memory_order_relaxed)) {

              // Try to adjust the remove pointer. It does not matter if 
              // this CAS fails.
              right_[buffer_index]->compare_exchange_weak(
                  old_right, (Item*)add_next_aba(result, old_right, 0), 
                  std::memory_order_seq_cst, std::memory_order_relaxed);

              *element = result->data.load(std::memory_order_seq_cst);
              return true;
            } else {
            }
          }
        } else {
          // No element was found, work on the emptiness check.
          if (emptiness_check_right[tmp_buffer_index] 
              != tmp_right) {
            empty = false;
            emptiness_check_right[tmp_buffer_index] = 
              tmp_right;
          }
          Item* tmp_left = left_[tmp_buffer_index]->load();
          if (emptiness_check_left[tmp_buffer_index] 
              != tmp_left) {
            empty = false;
            emptiness_check_left[tmp_buffer_index] = 
              tmp_left;
          }
        }
      }
      if (result != NULL) {
        if (timestamp == *threshold) {
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
          if (result->taken.compare_exchange_weak(
                  expected, 1, 
                  std::memory_order_seq_cst, std::memory_order_relaxed)) {
            // Try to adjust the remove pointer. It does not matter if this 
            // CAS fails.
            right_[buffer_index]->compare_exchange_weak(
                old_right, (Item*)add_next_aba(result, old_right, 0), 
                std::memory_order_seq_cst, std::memory_order_relaxed);
            *element = result->data.load(std::memory_order_seq_cst);
            return true;
          }
        }
        *threshold = result->timestamp.load();
      }

      *element = (T)NULL;
      return !empty;
    }
};

template<typename T>
class TSDeque : public Deque<T> {
 private:
  TSDequeBuffer<T> *buffer_;
  TimeStamp *timestamping_;
  bool init_threshold_;
 public:
  explicit TSDeque
    (TSDequeBuffer<T> *buffer, TimeStamp *timestamping, bool init_threshold) 
    : buffer_(buffer), timestamping_(timestamping),
      init_threshold_(init_threshold) {
  }
  bool insert_left(T item);
  bool remove_left(T *item);
  bool insert_right(T item);
  bool remove_right(T *item); 
};

template<typename T>
bool TSDeque<T>::insert_left(T element) {
  uint64_t timestamp = timestamping_->get_timestamp();
  buffer_->insert_left(element, timestamp);
  return true;
}

template<typename T>
bool TSDeque<T>::insert_right(T element) {
  uint64_t timestamp = timestamping_->get_timestamp();
  buffer_->insert_right(element, timestamp);
  return true;
}

template<typename T>
bool TSDeque<T>::remove_left(T *element) {
  int64_t threshold;
  if (init_threshold_) {
    threshold = ((int64_t)timestamping_->get_timestamp()) * -1;
  } else {
    threshold = INT64_MIN;
  }

  while (buffer_->try_remove_left(element, &threshold)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool TSDeque<T>::remove_right(T *element) {
  int64_t threshold;
  if (init_threshold_) {
    threshold = timestamping_->get_timestamp();
  } else {
    threshold = INT64_MAX;
  }
  while (buffer_->try_remove_right(element, &threshold)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_DEQUE_H_
