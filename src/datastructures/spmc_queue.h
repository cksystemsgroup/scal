#ifndef SCAL_DATASTRUCTURES_SPMC_QUEUE_H_
#define SCAL_DATASTRUCTURES_SPMC_QUEUE_H_

#include <stdint.h>
#include <assert.h>
#include <atomic>

#include "util/malloc.h"
#include "util/platform.h"
#include "datastructures/queue.h"
#include "util/threadlocals.h"
#include "datastructures/partial_pool_interface.h"
#include "util/atomic_value.h"

#define BUFFERSIZE 1000000

namespace spmc_details {
  // An item contains the element, a time stamp when the element was enqueued,
  // and a flag which indicates if the element has already been dequeued.
  template<typename T>
    struct Node {
      T value;
      std::atomic<Node<T>*> next;
      std::atomic<uint64_t> taken;
    };
}

template<typename T>
class SPMCQueue : public Queue<T>, PartialPoolInterface {
 public:
  explicit SPMCQueue();
  bool enqueue(T item);
  bool dequeue(T *item);

  inline uint64_t approx_size(void) const {
    return 0;
  }

  inline bool get_return_empty_state(T *item, AtomicRaw *state) {
    return dequeue_return_tail(item, state);
  }

  inline AtomicRaw empty_state() {
   return (AtomicRaw)(remove_.load());
  }

 private:
  typedef spmc_details::Node<T> Node;
  // The number of threads.
  // The insert pointers of the thread-local queues.
  std::atomic<Node*> insert_;
  // The remove pointers of the thread-local queues.
  std::atomic<Node*> remove_;

  bool dequeue_return_tail(T *item, AtomicRaw *tail_raw);

  inline Node* node_new(T item) const {
    Node *node = scal::tlget_aligned<Node>(scal::kCachePrefetch);
    node->next.store(node, std::memory_order_release);
    node->taken.store(0, std::memory_order_release);
    node->value = item;
    return node;
  }
};

template<typename T>
SPMCQueue<T>::SPMCQueue() {
  Node *sentinel = node_new(0);
  sentinel->taken.store(1, std::memory_order_seq_cst);
  sentinel->next.store(sentinel, std::memory_order_seq_cst);
  insert_.store(sentinel, std::memory_order_seq_cst);
  remove_.store(sentinel, std::memory_order_seq_cst);
}

template<typename T>
bool SPMCQueue<T>::enqueue(T element) {
  Node* new_node = node_new(element);
  insert_.load()->next.store(new_node);
  insert_.store(new_node);
  return true;
}

template<typename T>
bool SPMCQueue<T>::dequeue(T *element) {
  *element = 0;
  Node *remove = remove_.load();
  Node *node = remove;

  while (true) {
    if (node->taken.load() == 0) {
      uint64_t expected = 0;
      if (node->taken.compare_exchange_weak(expected, 1)) {
        *element = node->value;
        node = node->next.load();
        break;
      }
    }
    
    Node* next = node->next.load();
    if (next == node) {
      break;
    }
    node = next;
  }

  if (remove != node) {
    remove_.compare_exchange_weak(remove, node);
  }
  if (*element == 0) {
    return false;
  }
  return true;
}

template<typename T>
bool SPMCQueue<T>::dequeue_return_tail(T *element, AtomicRaw *tail_raw) { *element = 0;
  Node *remove = remove_.load();
  Node *node = remove;

  while (true) {
    if (node->taken.load() == 0) {
      uint64_t expected = 0;
      if (node->taken.compare_exchange_weak(expected, 1)) {
        *element = node->value;
        node = node->next.load();
        break;
      }
    }
    
    Node* next = node->next.load();
    if (next == node) {
      break;
    }
    node = next;
  }

  if (remove != node) {
    remove_.compare_exchange_weak(remove, node);
  }
  *tail_raw = (AtomicRaw)node;
  if (*element == 0) {
    return false;
  }
  return true;
}

#endif  // SCAL_DATASTRUCTURES_SPMC_QUEUE_H_
