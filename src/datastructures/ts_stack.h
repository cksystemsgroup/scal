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

#define BUFFERSIZE 1000000
#define ABAOFFSET (1ul << 32)

namespace ts_details {
  // An item contains the element, a time stamp when the element was enqueued,
  // and a flag which indicates if the element has already been dequeued.
  template<typename T>
    struct Item {
      std::atomic<T> data;
      std::atomic<uint64_t> timestamp;
      std::atomic<uint64_t> taken;
    };
}

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

template<typename T>
class TLArrayBuffer : public TSBuffer<T> {
  public:
    void insert_element(T element, uint64_t timestamp) {
    };
    bool try_remove_youngest(T *element, uint64_t *threshold) {
      return false;
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
  uint64_t threshold = 0;
  while (buffer_->try_remove_youngest(element, &threshold)) {
    if (element != (T)NULL) {
      return true;
    }
  }
  return false;
}
#endif  // SCAL_DATASTRUCTURES_TS_Stack_H_
