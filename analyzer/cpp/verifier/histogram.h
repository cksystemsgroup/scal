#ifndef HISTOGRAM_H_
#define HISTOGRAM_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <math.h>

#define HISTOGRAMSIZE (100*1024)

class Histogram {
 public:
  Histogram() {
    for(int i=0; i<HISTOGRAMSIZE; i++) {
      values[i] = 0;
    }
  }
  ~Histogram() {}

  void add(uint64_t value) {
    if (value >= HISTOGRAMSIZE) {
      printf("histogram overflow\n");
      values[HISTOGRAMSIZE-1]++;
    } else{
      values[value]++;
    }
  }

  uint64_t commulativeError() {
    uint64_t sum = 0;
    for (int i=1; i<HISTOGRAMSIZE; i++) {
      sum += i* values[i];
    }
    return sum;
  }

  uint64_t numberOfSamples() {
    uint64_t sum = 0;
    for (int i=0; i<HISTOGRAMSIZE; i++) {
      sum += values[i];
    }
    return sum;
  }

  int errors() {
    int sum = 0;
    for (int i=0; i<HISTOGRAMSIZE; i++) {
      if (values[i] != 0) {
        sum++;
      }
    }
    return sum;
  }

  uint64_t max() {
    uint64_t maxValue = -1;
    for (int i=0; i<HISTOGRAMSIZE; i++) {
      if (values[i] != 0) {
        maxValue = i;
      }
    }
    return maxValue;
  }

  float mean() {
    return (float)commulativeError() / (float)numberOfSamples();
  }

  float stdv() {
    float result = 0.0;
    float meanValue = mean();
    for (int i=0; i<HISTOGRAMSIZE; i++) {
      if (values[i] != 0) {
        result += pow(i - meanValue, 2) * values[i];
      }
    }
    result /= (float)numberOfSamples();
    return sqrt(result);
  }

  void print() {
    for (int i=0; i<HISTOGRAMSIZE; i++) {
      if (values[i] != 0) {
        printf("%d: %" PRIu64 "\n", i, values[i]);
      }
    }
  }
  
 private:
  uint64_t values[HISTOGRAMSIZE];
};

#endif  // HISTOGRAM_H_
