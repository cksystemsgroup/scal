#ifndef SCAL_DATASTRUCTURES_TS_DEQUE_H_
#define SCAL_DATASTRUCTURES_TS_DEQUE_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <time.h>

#include "datastructures/deque.h"
#include "datastructures/ts_timestamp.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"
#include "util/random.h"
#include "util/workloads.h"

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

//////////////////////////////////////////////////////////////////////
// The base TSDequeBuffer class.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TSDequeBuffer {
  public:
    virtual void insert_left(T element, TimeStamp *timestamp) = 0;
    virtual void insert_right(T element, TimeStamp *timestamp) = 0;
    virtual bool try_remove_left
      (T *element, int64_t *threshold, void **potential_element) = 0;
    virtual bool try_remove_right
      (T *element, int64_t *threshold, void ** potential_element) = 0;
};

enum DequeSide {
  kLeft = 0,
  kRight = 1,
  kRandom = 2
};

//////////////////////////////////////////////////////////////////////
// A TSDequeBuffer based on thread-local linked lists.
//////////////////////////////////////////////////////////////////////
template<typename T>
class TL2TSDequeBuffer : public TSDequeBuffer<T> {
  private:
    typedef struct Item {
      std::atomic<Item*> left;
      std::atomic<Item*> right;
      std::atomic<uint64_t> taken;
      std::atomic<T> data;
      std::atomic<int64_t> t1;
      std::atomic<int64_t> t2;
    } Item;

    // The number of threads.
    uint64_t num_threads_;
    std::atomic<Item*> **left_;
    std::atomic<Item*> **right_;
    // The pointers for the emptiness check.
    Item** *emptiness_check_left_;
    Item** *emptiness_check_right_;
    uint64_t delay_;

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

      Item* old_right = right_[thread_id]->load();
      Item* right = (Item*)get_aba_free_pointer(old_right);

      int64_t threshold = right->t1.load();

      Item* result = (Item*)get_aba_free_pointer(left_[thread_id]->load());

      // We start at the left pointer and iterate to the right until we
      // find the first item which has not been taken yet.
      while (result->taken.load() != 0 &&
          result->right.load() != result) {
        result = result->right.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the 
      // problem that we may find an element inserted at the right side 
      // which is older than an element which was inserted on the left 
      // side in the meantime.
      // 
      // The interpretation of the second clause in the condition is the
      // following: if the t1-time stamp of the result is greater than
      // the threshold, then the element was inserted during the
      // execution of this method. We cannot take the element because
      // there may also exist a new element inserted at the left side
      // in the meantime and we would have to prefer than one.
      // Additionally we check if the result is different to the right
      // pointer we read at the beginning of the method because if
      // both right and result do not have a t1 time stamp at the time
      // we read the t1 time stamp, then they both have the same time
      // stamp but we are not allowed to take result except if it is
      // the same as the one we read in the beginning of the method. 
      if (result->taken.load() != 0 ||
         (result->t1.load() >= threshold &&
          result != right)) {
        return NULL;
      }
      return result;
    }

    Item* get_right_item(uint64_t thread_id) {

      Item* old_left = left_[thread_id]->load();
      Item* left = (Item*)get_aba_free_pointer(old_left);

      int64_t threshold = left->t2.load();

      Item* result = (Item*)get_aba_free_pointer(
        right_[thread_id]->load());

      // We start at the right pointer and iterate to the left until we
      // find the first item which has not been taken yet.
      while (result->taken.load() != 0 &&
          result->left.load() != result) {
        result = result->left.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the
      // problem that we may find an element inserted at the left side
      // which is older than an element which was inserted on the right
      // side in the meantime.
      // 
      // The interpretation of the second clause in the condition is the
      // following: if the t2-time stamp of the result is less than
      // the threshold, then the element was inserted during the
      // execution of this method. We cannot take the element because
      // there may also exist a new element inserted at the right side
      // in the meantime and we would have to prefer than one.
      // Additionally we check if the result is different to the left
      // pointer we read at the beginning of the method because if
      // both left and result do not have a t1 time stamp at the time
      // we read the t1 time stamp, then they both have the same time
      // stamp but we are not allowed to take result except if it is
      // the same as the one we read in the beginning of the method. 
      if (result->taken.load() != 0 || 
          (result->t2.load() <= threshold && result != left)) {
        return NULL;
      } else {
        return result;
      }
    }

  public:
    /////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////
    TL2TSDequeBuffer(uint64_t num_threads, uint64_t delay) 
      : num_threads_(num_threads), delay_(delay) {
      left_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      right_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      emptiness_check_left_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      emptiness_check_right_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        left_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        right_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        new_item->t1.store(0);
        new_item->t2.store(0);
        new_item->data.store(0);
        new_item->taken.store(1);
        new_item->left.store(new_item);
        new_item->right.store(new_item);
        left_[i]->store(new_item);
        right_[i]->store(new_item);

        emptiness_check_left_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));

        emptiness_check_right_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }
    }

    /////////////////////////////////////////////////////////////////
    // insert_left
    /////////////////////////////////////////////////////////////////
    void insert_left(T element, TimeStamp *timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      // Switch the sign of the time stamp of elements inserted at the
      // left side.
      new_item->t1.store(INT64_MIN);
      new_item->t2.store(INT64_MIN);
      new_item->data.store(element);
      new_item->taken.store(0);
      new_item->left.store(new_item);

      Item* old_left = left_[thread_id]->load();

      Item* left = (Item*)get_aba_free_pointer(old_left);
      while (left->right.load() != left 
          && left->taken.load()) {
        left = left->right.load();
      }

      if (left->right.load() == left) {
        // The buffer is empty. We have to increase the aba counter of the
        // right pointer too to guarantee that a pending right-pointer
        // update of a remove operation does not make the left and the
        // right pointer point to different lists.
        Item* old_right = right_[thread_id]->load();
        right_[thread_id]->store((Item*) add_next_aba(left, old_right, 1));
      }

      new_item->right.store(left);
      left->left.store(new_item);
      left_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_left, 1));
 
      new_item->t2.store(((int64_t)timestamp->get_timestamp()) * (-1));
        
      uint64_t wait = get_hwtime() + delay_;
      while (get_hwtime() < wait) {}
//      struct timespec tim, tim2;
//      tim.tv_sec = 0;
//      tim.tv_nsec = delay_;
//      if(nanosleep(&tim , &tim2) < 0 ) {
//        printf("Nano sleep system call failed \n");
//        abort();
//      }
//      calculate_pi(delay_);
      new_item->t1.store(((int64_t)timestamp->get_timestamp()) * (-1));
    }

    /////////////////////////////////////////////////////////////////
    // insert_right
    /////////////////////////////////////////////////////////////////
    void insert_right(T element, TimeStamp *timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->t1.store(INT64_MAX);
      new_item->t2.store(INT64_MAX);
      new_item->data.store(element);
      new_item->taken.store(0);
      new_item->right.store(new_item);

      Item* old_right = right_[thread_id]->load();

      Item* right = (Item*)get_aba_free_pointer(old_right);
      while (right->left.load() != right 
          && right->taken.load()) {
        right = right->left.load();
      }

      if (right->left.load() == right) {
        // The buffer is empty. We have to increase the aba counter of the
        // left pointer too to guarantee that a pending left-pointer
        // update of a remove operation does not make the left and the
        // right pointer point to different lists.
        Item* old_left = left_[thread_id]->load();
        left_[thread_id]->store( (Item*) add_next_aba(right, old_left, 1)); }

      new_item->left.store(right);
      right->right.store(new_item);
      right_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_right, 1));

      new_item->t1.store((int64_t)timestamp->get_timestamp());

      uint64_t wait = get_hwtime() + delay_;
      while (get_hwtime() < wait) {}
//      struct timespec tim, tim2;
//      tim.tv_sec = 0;
//      tim.tv_nsec = delay_;
//      if(nanosleep(&tim , &tim2) < 0 ) {
//        printf("Nano sleep system call failed \n");
//        abort();
//      }
//      calculate_pi(delay_);
      new_item->t2.store((int64_t)timestamp->get_timestamp());
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_left
    /////////////////////////////////////////////////////////////////
    bool try_remove_left
      (T *element, int64_t *threshold, void ** potential_element) {
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
      int64_t t1 = INT64_MAX;
      int64_t t2 = INT64_MAX;
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
          int64_t item_t1 = item->t1.load();
          int64_t item_t2 = item->t2.load();

          if (t1 > item_t2 || 
              (t1 == INT64_MAX && item == *potential_element)) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            t1 = item_t1;
            t2 = item_t2;
            old_left = tmp_left;
           
            // Check if we can remove the element immediately.
            if (threshold[0] > t1) {
              uint64_t expected = 0;
              if (result->taken.load() == 0) {
                if (result->taken.compare_exchange_weak(
                    expected, 1)) {
                  // Try to adjust the remove pointer. It does not matter if 
                  // this CAS fails.
                  left_[buffer_index]->compare_exchange_weak(
                      old_left, (Item*)add_next_aba(result, old_left, 0));

                  *element = result->data.load();
                  return true;
                }
              }
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
        // (t1 == INT64_MAX) means that the element was inserted at the
        // right side and did not get a time stamp yet. We should not take
        // it then, except if we found the same element already in the
        // previous iteration (and stored it as potential_element).
        if ((t1 != INT64_MAX &&t1 <= threshold[0]) 
            || result == *potential_element
            || (t1 > 0 && t1 < threshold[1])
            || (t2 < 0 && t2 > -threshold[1])
            ){
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
          if (result->taken.load() == 0) {
            if (result->taken.compare_exchange_weak(
                    expected, 1)) {
              // Try to adjust the remove pointer. It does not matter if this 
              // CAS fails.
              left_[buffer_index]->compare_exchange_weak(
                  old_left, (Item*)add_next_aba(result, old_left, 0));
              *element = result->data.load();
              return true;
            }
          }
        }
        // We only set an new threshold if the element we tried to remove
        // already had a time stamp. Otherwise we continue to use the old
        // time stamp. 
        if (t1 != INT64_MAX) {
          threshold[0] =  t1;
          *potential_element = NULL;
        } else {
          *potential_element = result;
        }
      }

      *element = (T)NULL;
      return !empty;
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_right
    ////////////////////////////////////////////////////////////////
    bool try_remove_right
      (T *element, int64_t *threshold, void ** potential_element) {
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
      int64_t t1 = INT64_MIN;
      int64_t t2 = INT64_MIN;
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
          int64_t item_t1 = item->t1.load();
          int64_t item_t2 = item->t2.load();
          if (t2 < item_t1 || 
             (t2 == INT64_MIN && item == *potential_element)) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            t1 = item_t1;
            t2 = item_t2;
            old_right = tmp_right;

            // Check if we can remove the element immediately.
            if (threshold[0] < t2) {
              uint64_t expected = 0;
              if (result->taken.load() == 0) {
                if (result->taken.compare_exchange_weak(
                      expected, 1)) {

                  // Try to adjust the remove pointer. It does not matter if 
                  // this CAS fails.
                  right_[buffer_index]->compare_exchange_weak(
                      old_right, (Item*)add_next_aba(result, old_right, 0));

                  *element = result->data.load();
                  return true;
                }
              }
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
        // (t2 == INT64_MIN) means that the element was inserted at the
        // left side and did not get a time stamp yet. We should not take
        // it then, except if we found the same element already in the
        // previous iteration (and stored it as potential_element).
        if ((t2 != INT64_MIN && t2 >= threshold[0]) 
            || (result == *potential_element)
            || (t1 > 0 && t1 < threshold[1])
            || (t2 < 0 && t2 > -threshold[1])
            ) {
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
          if (result->taken.load() == 0) {
            if (result->taken.compare_exchange_weak(
                    expected, 1)) {
              // Try to adjust the remove pointer. It does not matter if
              // this CAS fails.
              right_[buffer_index]->compare_exchange_weak(
                  old_right, (Item*)add_next_aba(result, old_right, 0));
              *element = result->data.load();
              return true;
            }
          }
        }
        // We only set an new threshold if the element we tried to remove
        // already had a time stamp. Otherwise we continue to use the old
        // time stamp. 
        if (t2 != INT64_MIN) {
          threshold[0] =  t2;
          *potential_element = NULL;
        } else {
          *potential_element = result;
        }
      }

      *element = (T)NULL;
      return !empty;
    }
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

      Item* old_right = right_[thread_id]->load();
      Item* right = (Item*)get_aba_free_pointer(old_right);

      int64_t threshold = right->timestamp.load();

      Item* result = (Item*)get_aba_free_pointer(left_[thread_id]->load());

      // We start at the left pointer and iterate to the right until we
      // find the first item which has not been taken yet.
      while (result->taken.load() != 0 &&
          result->right.load() != result) {
        result = result->right.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the
      // problem that we may find an element inserted at the right side
      // which is older than an element which was inserted on the left
      // side in the meantime.
      // 
      // The interpretation of the second clause in the condition is the
      // following: if the t1-time stamp of the result is greater than
      // the threshold, then the element was inserted during the
      // execution of this method. We cannot take the element because
      // there may also exist a new element inserted at the left side
      // in the meantime and we would have to prefer than one.
      // Additionally we check if the result is different to the right
      // pointer we read at the beginning of the method because if
      // both right and result do not have a t1 time stamp at the time
      // we read the t1 time stamp, then they both have the same time
      // stamp but we are not allowed to take result except if it is
      // the same as the one we read in the beginning of the method. 
      if (result->taken.load() != 0 ||
         (result->timestamp.load() >= threshold &&
          result != right)) {
        return NULL;
      }
      return result;
    }

    Item* get_right_item(uint64_t thread_id) {

      Item* left = (Item*)get_aba_free_pointer(
          left_[thread_id]->load());

      int64_t threshold = left->timestamp.load();

      Item* result = (Item*)get_aba_free_pointer(
        right_[thread_id]->load());

      // We start at the right pointer and iterate to the left until we find
      // the first item which has not been taken yet.
      while (result->taken.load() != 0 &&
          result->left.load() != result) {
        result = result->left.load();
      }

      // We don't return the element if it was taken already or if it
      // was inserted after we started the search. Otherwise we have the
      // problem that we may find an element inserted at the left side
      // which is older than an element which was inserted on the right
      // side in the meantime.
      // 
      // The interpretation of the second clause in the condition is the
      // following: if the t2-time stamp of the result is less than
      // the threshold, then the element was inserted during the
      // execution of this method. We cannot take the element because
      // there may also exist a new element inserted at the right side
      // in the meantime and we would have to prefer than one.
      // Additionally we check if the result is different to the left
      // pointer we read at the beginning of the method because if
      // both left and result do not have a t1 time stamp at the time
      // we read the t1 time stamp, then they both have the same time
      // stamp but we are not allowed to take result except if it is
      // the same as the one we read in the beginning of the method. 
      if (result->taken.load() != 0 || 
          result->timestamp.load() <= threshold && result != left) {
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
            scal::kCachePrefetch * 4));

      right_ = static_cast<std::atomic<Item*>**>(
          scal::calloc_aligned(num_threads_, sizeof(std::atomic<Item*>*), 
            scal::kCachePrefetch * 4));

      emptiness_check_left_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      emptiness_check_right_ = static_cast<Item***>(
          scal::calloc_aligned(num_threads_, sizeof(Item**), 
            scal::kCachePrefetch * 4));

      for (int i = 0; i < num_threads_; i++) {

        left_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        right_[i] = static_cast<std::atomic<Item*>*>(
            scal::get<std::atomic<Item*>>(scal::kCachePrefetch * 4));

        // Add a sentinal node.
        Item *new_item = scal::get<Item>(scal::kCachePrefetch * 4);
        new_item->timestamp.store(0);
        new_item->data.store(0);
        new_item->taken.store(1);
        new_item->left.store(new_item);
        new_item->right.store(new_item);
        left_[i]->store(new_item);
        right_[i]->store(new_item);

        emptiness_check_left_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));

        emptiness_check_right_[i] = static_cast<Item**> (
            scal::calloc_aligned(num_threads_, sizeof(Item*), 
              scal::kCachePrefetch * 4));
      }
    }

    /////////////////////////////////////////////////////////////////
    // insert_left
    /////////////////////////////////////////////////////////////////
    void insert_left(T element, TimeStamp *timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      // Switch the sign of the time stamp of elements inserted at the left side.
      new_item->timestamp.store(
          ((int64_t)timestamp->get_timestamp()) * (-1));
      new_item->data.store(element);
      new_item->taken.store(0);
      new_item->left.store(new_item);

      Item* old_left = left_[thread_id]->load();

      Item* left = (Item*)get_aba_free_pointer(old_left);
      while (left->right.load() != left 
          && left->taken.load()) {
        left = left->right.load();
      }

      if (left->right.load() == left) {
        // The buffer is empty. We have to increase the aba counter of the right
        // pointer too to guarantee that a pending right-pointer update of a
        // remove operation does not make the left and the right pointer point
        // to different lists.
        Item* old_right = right_[thread_id]->load();
        right_[thread_id]->store( (Item*) add_next_aba(left, old_right, 1));
      }

      new_item->right.store(left);
      left->left.store(new_item);
      left_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_left, 1));
    }

    /////////////////////////////////////////////////////////////////
    // insert_right
    /////////////////////////////////////////////////////////////////
    void insert_right(T element, TimeStamp *timestamp) {
      uint64_t thread_id = scal::ThreadContext::get().thread_id();

      Item *new_item = scal::tlget_aligned<Item>(scal::kCachePrefetch);
      new_item->timestamp.store(timestamp->get_timestamp());
      new_item->data.store(element);
      new_item->taken.store(0);
      new_item->right.store(new_item);

      Item* old_right = right_[thread_id]->load();

      Item* right = (Item*)get_aba_free_pointer(old_right);
      while (right->left.load() != right 
          && right->taken.load()) {
        right = right->left.load();
      }

      if (right->left.load() == right) {
        // The buffer is empty. We have to increase the aba counter of the left
        // pointer too to guarantee that a pending left-pointer update of a
        // remove operation does not make the left and the right pointer point
        // to different lists.
        Item* old_left = left_[thread_id]->load();
        left_[thread_id]->store( (Item*) add_next_aba(right, old_left, 1)); }

      new_item->left.store(right);
      right->right.store(new_item);
      right_[thread_id]->store(
        (Item*) add_next_aba(new_item, old_right, 1));
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_left
    /////////////////////////////////////////////////////////////////
    bool try_remove_left
      (T *element, int64_t *threshold, void ** potential_element) {
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
            item->timestamp.load();

          if (timestamp > item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_left = tmp_left;
          } 
          // Check if we can remove the element immediately.
          if (threshold[0] > timestamp) {
            uint64_t expected = 0;
            if (result->taken.load() == 0) {
              if (result->taken.compare_exchange_weak(
                  expected, 1)) {
                // Try to adjust the remove pointer. It does not matter if 
                // this CAS fails.
                left_[buffer_index]->compare_exchange_weak(
                    old_left, (Item*)add_next_aba(result, old_left, 0));

                *element = result->data.load();
                return true;
              }
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
        // (timestamp == INT64_MAX) means that the element was inserted at
        // the right side and did not get a time stamp yet. We should not
        // take it then, except if we found the same element already in the
        // previous iteration (and stored it as potential_element).
        if ((timestamp != INT64_MAX && timestamp <= threshold[0])
            || result == *potential_element
            || (timestamp > 0 && timestamp < threshold[1])
            || (timestamp < 0 && timestamp > -threshold[1])
            ){
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
            if (result->taken.load() == 0) {
          if (result->taken.compare_exchange_weak(
                  expected, 1)) {
            // Try to adjust the remove pointer. It does not matter if this 
            // CAS fails.
            left_[buffer_index]->compare_exchange_weak(
                old_left, (Item*)add_next_aba(result, old_left, 0));
            *element = result->data.load();
            return true;
          }
            }
        }
        // We only set an new threshold if the element we tried to remove
        // already had a time stamp. Otherwise we continue to use the old
        // time stamp. 
        if (timestamp != INT64_MAX) {
          threshold[0] =  timestamp;
          *potential_element = NULL;
        } else {
          *potential_element = result;
        }
      }

      *element = (T)NULL;
      return !empty;
    }

    /////////////////////////////////////////////////////////////////
    // try_remove_right
    ////////////////////////////////////////////////////////////////
    bool try_remove_right
      (T *element, int64_t *threshold, void ** potential_element) {
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
            item->timestamp.load();
          if (timestamp < item_timestamp) {
            // We found a new youngest element, so we remember it.
            result = item;
            buffer_index = tmp_buffer_index;
            timestamp = item_timestamp;
            old_right = tmp_right;
          }
          // Check if we can remove the element immediately.
          if (threshold[0] < timestamp) {
            uint64_t expected = 0;
            if (result->taken.load() == 0) {
              if (result->taken.compare_exchange_weak(
                    expected, 1)) {

                // Try to adjust the remove pointer. It does not matter if 
                // this CAS fails.
                right_[buffer_index]->compare_exchange_weak(
                    old_right, (Item*)add_next_aba(result, old_right, 0));

                *element = result->data.load();
                return true;
              }
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
        // (timestamp == INT64_MIN) means that the element was inserted at
        // the left side and did not get a time stamp yet. We should not
        // take it then, except if we found the same element already in the
        // previous iteration (and stored it as potential_element).
        if ((timestamp != INT64_MIN && timestamp >= threshold[0])
            || result == *potential_element
            || (timestamp > 0 && timestamp < threshold[1])
            || (timestamp < 0 && timestamp > -threshold[1])
            ){
          // We found a similar element to the one in the last iteration. Let's
          // try to remove it
          uint64_t expected = 0;
          if (result->taken.load() == 0) {
            if (result->taken.compare_exchange_weak(
                    expected, 1)) {
              // Try to adjust the remove pointer. It does not matter if
              // this CAS fails.
              right_[buffer_index]->compare_exchange_weak(
                  old_right, (Item*)add_next_aba(result, old_right, 0));
              *element = result->data.load();
              return true;
            }
          }
        }
        // We only set an new threshold if the element we tried to remove
        // already had a time stamp. Otherwise we continue to use the old
        // time stamp. 
        if (timestamp != INT64_MIN) {
          threshold[0] = timestamp;
          *potential_element = NULL;
        } else {
          *potential_element = result;
        }
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
    (TSDequeBuffer<T> *buffer, TimeStamp *timestamping, 
     bool init_threshold)
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
  buffer_->insert_left(element, timestamping_);
  return true;
}

template<typename T>
bool TSDeque<T>::insert_right(T element) {
  buffer_->insert_right(element, timestamping_);
  return true;
}

template<typename T>
bool TSDeque<T>::remove_left(T *element) {
  int64_t threshold[2];
  if (init_threshold_) {
    //threshold[1] is positive because defines a point in time independent
    //of the side of the insert.
    threshold[1] = timestamping_->read_time();
    threshold[0] = -threshold[1];
  } else {
    threshold[0] = INT64_MIN;
    //threshold[1] is INT64_MIN because we want any insert operation to
    //start after threshold[1].
    threshold[1] = INT64_MIN;
  }

  void *potential_element = NULL;

  while (
    buffer_->try_remove_left(element, threshold, &potential_element)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}

template<typename T>
bool TSDeque<T>::remove_right(T *element) {
  int64_t threshold[2];
  if (init_threshold_) {
    //threshold[1] is positive because defines a point in time independent
    //of the side of the insert.
    threshold[1] = timestamping_->read_time();
    threshold[0] = threshold[1];
  } else {
    threshold[0] = INT64_MAX;
    //threshold[1] is INT64_MIN because we want any insert operation to
    //start after threshold[1].
    threshold[1] = INT64_MIN;
  }

  void *potential_element = NULL;

  while (
    buffer_->try_remove_right(element, threshold, &potential_element)) {
    if (*element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_DEQUE_H_
