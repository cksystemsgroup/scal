#ifndef SCAL_DATASTRUCTURES_TS_TIMESTAMP_H_
#define SCAL_DATASTRUCTURES_TS_TIMESTAMP_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <stdio.h>

#include "util/threadlocals.h"
#include "util/time.h"
#include "util/malloc.h"
#include "util/platform.h"

//////////////////////////////////////////////////////////////////////
// The base TimeStamp class.
//////////////////////////////////////////////////////////////////////
class TimeStamp {
 public:
  virtual uint64_t get_timestamp() = 0;
  virtual uint64_t read_time() = 0;
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
            sizeof(std::atomic<uint64_t>*), scal::kCachePrefetch));

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

    //////////////////////////////////////////////////////////////////////
    // Read the current time.
    //////////////////////////////////////////////////////////////////////
    uint64_t read_time() {

      uint64_t thread_id = scal::ThreadContext::get().thread_id();
      uint64_t latest_time = 0;

      // Find the latest of all thread-local times.
      for (int i = 0; i < num_threads_; i++) {
        latest_time = 
          max(latest_time, clocks_[i]->load(std::memory_order_acquire));
      }

      // Return the current local time.
      return latest_time;
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
      clock_ = scal::get<std::atomic<uint64_t>>(scal::kCachePrefetch * 4);
      clock_->store(1);
    }
  
    uint64_t get_timestamp() {
      return clock_->fetch_add(1);
    }

    uint64_t read_time() {
      return clock_->load();
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
    uint64_t read_time() {
      return get_hwtime();
    }
};

//////////////////////////////////////////////////////////////////////
// A TimeStamp class based on a hardware counter
//////////////////////////////////////////////////////////////////////
class ShiftedHardwareTimeStamp : public TimeStamp {
  private:


  public:
    ShiftedHardwareTimeStamp() {
    }
  
    uint64_t get_timestamp() {
      return get_hwtime() >> 1;
    }
    uint64_t read_time() {
      return get_hwtime() >> 1;
    }
};
#endif // SCAL_DATASTRUCTURES_TS_TIMESTAMP_H_
