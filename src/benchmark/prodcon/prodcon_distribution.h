#ifndef SCAL_BENCHMARK_PRODCON_PRODCON_DISTRIBUTION_H_
#define SCAL_BENCHMARK_PRODCON_PRODCON_DISTRIBUTION_H_

#include <gflags/gflags.h>
#include <inttypes.h>

#include <util/random.h>

DEFINE_uint64(shuffle_threads_seed, 0, "seed for thread_shuffle");
DEFINE_bool(shuffle_threads_print, false, "print permutation");

namespace scal {

class ProdConDistribution {
 public:
  ProdConDistribution(size_t producer, size_t consumer)
      : producer_(producer)
      , consumer_(consumer) {}

  virtual bool IsProducer(size_t thread_id) = 0;

 protected:
  size_t num_threads() {
    return producer_ + consumer_;
  }

  size_t producer_;
  size_t consumer_;
};


class DefaultProdConDistribution  : public ProdConDistribution {
 public:
  DefaultProdConDistribution(size_t producer, size_t consumer)
      : ProdConDistribution(producer, consumer) {}

  virtual bool IsProducer(size_t thread_id) {
    if (thread_id < producer_) {
      return true;
    }
    return false;
  }
};


class RandomProdConDistribution : public ProdConDistribution {
 public:
  RandomProdConDistribution(size_t producer, size_t consumer)
      : ProdConDistribution(producer, consumer) {
    decider_ = new int[num_threads()];
    for (size_t i = 0; i < producer_; i++) {
      decider_[i] = 1;
    }
    for (size_t i = 0; i < consumer_; i++) {
      decider_[i + producer_] = 0;
    }
    scal::shuffle<int>(decider_, num_threads(), FLAGS_shuffle_threads_seed);
    size_t producers = 0;
    size_t consumers = 0;
    for (size_t i = 0; i < num_threads(); i++) {
      if (decider_[i] == 0) {
        consumers++;
      } else {
        producers++;
      }
    }
    if ((producers != producer_) ||
        (consumers != consumer)) {
      fprintf(stderr, "inconsistent permutation for prodcon distribution\n");
      abort();
    }
    if (FLAGS_shuffle_threads_print) {
      printf("prodcon permutation: ");
      for (size_t i = 0; i < num_threads(); i++) {
        printf("%d ", decider_[i]);
      }
      printf("\n");
    }
  }

  virtual bool IsProducer(size_t thread_id) {
    if (decider_[thread_id] > 0) {
      return true;
    }
    return false;
  }

 private:
  int* decider_;
};

}  // namespace scal

#endif  // SCAL_BENCHMARK_PRODCON_PRODCON_DISTRIBUTION_H_

