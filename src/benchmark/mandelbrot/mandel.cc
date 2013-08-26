#include <errno.h>
#include <gflags/gflags.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "benchmark/common.h"
#include "benchmark/mandelbrot/bitmap.h"
#include "benchmark/mandelbrot/rgba.h"
#include "benchmark/std_glue/std_pipe_api.h"
#include "util/malloc.h"
#include "util/operation_logger.h"
#include "util/threadlocals.h"
#include "util/time.h"
#include "datastructures/pool.h"

DEFINE_uint64(prealloc_size, 65536 /* 256 MB */  * 20, "size in TLABs (most likely a page)");
DEFINE_bool(print_config, true, "print the Mandelbrot configuration");
DEFINE_double(x, 0, "x coordinate of image center");
DEFINE_double(y, 0, "y coordinate of image center");
DEFINE_uint64(max, 1000, "maximum iterations per Mandelbrot point");
DEFINE_double(scale, 4, "scale of the image in Mandelbrot coords");
DEFINE_uint64(width, 500, "width of the bitmap image");
DEFINE_uint64(height, 500, "height of the bitmap image");
DEFINE_string(output, "mb.bmp", "the output filename");
DEFINE_uint64(producers, 1, "number of producers");
DEFINE_uint64(consumers, 1, "number of consumers");
DEFINE_uint64(block_length, 1, "Mandelbrot block length in px");
DEFINE_uint64(block_height, 1, "Mandelbrot block height in px");
DEFINE_bool(generate_picture, false, "generate the picture?");

namespace {

using scal::Benchmark;
//using scal::BenchmarkArg;

struct Package {
  int x_start;
  int x_end;
  int y_start;
  int y_end;
};

struct MandelArg {
  void *ds;
  struct bitmap *bm;
  double xmin;
  double xmax;
  double xrange;
  double ymin;
  double ymax;
  double yrange;
  int work_flag;
};

class MandelbrotBench : public Benchmark {
 public:
  MandelbrotBench(uint64_t num_threads,
                  uint64_t thread_prealloc_size,
                  uint64_t histogram_size,
                  void *data) : Benchmark(
                    num_threads, thread_prealloc_size, data) {}
 protected:
  void bench_func(void);

 private:
  void worker_producer(MandelArg *, uint64_t);
  void worker_consumer(MandelArg *, uint64_t);
};

inline uint32_t iteration_to_color(int i, int max) {
  int gray = 255 * i / max;
  return rgba_create(gray, gray, gray, 0);
}

uint32_t mandelbrot_point(double x, double y, int max) {
  double xt;
  double yt;
  double x0 = x;
  double y0 = y;
  int i = 0;
  while ((x * x + y * y <= 4) && i < max) {
    xt = x * x - y * y + x0;
    yt = 2 * x * y + y0;
    x = xt;
    y = yt;
    i++;
  }
  return iteration_to_color(i, max);
}

void compute_block(MandelArg *targ, int x_start, int y_start, int x_end,
                          int y_end) {
  int i;
  int j;
  double x;
  double y;
  uint32_t color;
  for (i = x_start; i < x_end; i++) {
    for (j = y_start; j < y_end; j++) {
      x =  targ->xmin + i * targ->xrange / bitmap_width (targ->bm);
      y =  targ->ymin + j * targ->yrange / bitmap_height (targ->bm);
      color = mandelbrot_point(x, y, FLAGS_max);
      bitmap_set(targ->bm, i, j, color);
    }
  }
}

}  // namespace

uint64_t g_num_threads;

uint64_t g_work_flag __attribute__((aligned(128)));

int main (int argc, const char **argv) {
  std::string usage("compute a Mandelbort image in parallel\n\n  Sample usage:\n\
    mandel -x -0.5 -y -0.5 -s 0.2\n\
    mandel -x -.38 -y -.665 -s .05 -m 100\n\
    mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000");
  google::SetUsageMessage(usage);
  google::ParseCommandLineFlags(&argc, (char***)&argv, true);

  if (FLAGS_producers < 1) {
    fprintf (stderr, "atleast one producer is needed");
    abort();
  }
  if (FLAGS_consumers < 1) {
    fprintf (stderr, "atleast one consumer is needed");
    abort();
  }
  if (FLAGS_print_config) {
    printf ("mandel: x=%lf y=%lf scale=%lf max=%lu width=%lu height=%lu \
block_width=%lu block_height=%lu producer=%lu consumer=%lu outfile=%s\n", 
        FLAGS_x, FLAGS_y, FLAGS_scale, FLAGS_max, FLAGS_width,
        FLAGS_height, FLAGS_block_length, FLAGS_block_height, 
        FLAGS_producers, FLAGS_consumers, FLAGS_output.c_str());
  }

  g_num_threads = FLAGS_producers + FLAGS_consumers;

  scal::tlalloc_init(FLAGS_prealloc_size, true /* touch pages */);
//  threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

  const char *outfile = (const char*)FLAGS_output.c_str();

  scal::OperationLogger<uint64_t>::prepare(g_num_threads + 1, (FLAGS_height * FLAGS_width) / (FLAGS_block_length * FLAGS_block_height));

  struct bitmap *bm = bitmap_create(FLAGS_width, FLAGS_height);
  bitmap_reset(bm, rgba_create (0, 0, 255, 0));

  MandelArg targ;
  targ.bm = bm;
  targ.xmin = FLAGS_x - FLAGS_scale;
  targ.xmax = FLAGS_x + FLAGS_scale;
  targ.xrange = targ.xmax - targ.xmin;
  targ.ymin = FLAGS_y - FLAGS_scale;
  targ.ymax = FLAGS_y + FLAGS_scale;
  targ.yrange = targ.ymax - targ.ymin;
  targ.work_flag = FLAGS_producers;
  g_work_flag = FLAGS_producers;
  targ.ds = ds_new ();

  MandelbrotBench *benchmark = new MandelbrotBench(g_num_threads,
                                                   (FLAGS_prealloc_size / FLAGS_producers),
                                                   0,
                                                   &targ);
  benchmark->run();
  
  // print a summary: see summary.json for description
  uint64_t exec_time = benchmark->execution_time();
  printf("%lu %lu %lu %lu\n",
         FLAGS_producers + FLAGS_consumers,
         FLAGS_producers,
         FLAGS_consumers,
         exec_time);
  if (FLAGS_generate_picture && !bitmap_save (bm, outfile)) {
    fprintf(stderr,"%s: couldn't write to %s: %s\n", __func__, outfile, strerror(errno));
    abort();
  }
  return EXIT_SUCCESS;
}



void MandelbrotBench::bench_func(void) {
  struct sched_param param;
  uint64_t thread_id = scal::ThreadContext::get().thread_id();
  /*
  if (thread_id >= FLAGS_producers) {
    param.sched_priority = 39;
    if(pthread_setschedparam(pthread_self(), SCHED_RR, &param)) {
      perror("pthread_setschedparam");
    }
  } else {
    param.sched_priority = 41;
    if(pthread_setschedparam(pthread_self(), SCHED_RR, &param)) {
      perror("pthread_setschedparam");
    }
  }*/

  if (thread_id <= FLAGS_producers) {
    worker_producer(static_cast<MandelArg*>(data_), thread_id-1);
  } else {
    worker_consumer(static_cast<MandelArg*>(data_), thread_id-1);
  }
}

void MandelbrotBench::worker_consumer(MandelArg *targ, uint64_t thread_id) {
  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  Package *tmp;
  bool ret;
  int may_leave = false;
  uint64_t ret_value;
  while (true) {
    tmp = NULL;
    if (g_work_flag == 0) {
      may_leave = true;
    }
    ret = ds->get(&ret_value);
    if (!ret && may_leave) {
      break;
    }
    if (ret) {
      tmp = reinterpret_cast<Package*>(ret_value);
      compute_block(targ, tmp->x_start, tmp->y_start, tmp->x_end, tmp->y_end);
    } 
  }
}

void MandelbrotBench::worker_producer(MandelArg *targ, uint64_t thread_id) {
  using scal::tlcalloc_aligned;
  Pool<uint64_t> *ds = static_cast<Pool<uint64_t>*>(data_);
  int width = bitmap_width (targ->bm); 
  int height = bitmap_height (targ->bm);
  int i, j;
  int block_height = FLAGS_block_height;
  int height_step = block_height * FLAGS_producers;
  int block_length = FLAGS_block_length;
  int length_step = block_length;
  Package *tmp;
  for (j = block_height * thread_id; j < height; j += height_step) {
    for (i = 0; i < width; i+= length_step) {
      tmp = static_cast<Package*>(tlcalloc_aligned(1, sizeof(Package), 128));
      tmp->x_start = i;
      tmp->y_start = j;
      tmp->x_end = (i + block_length < width) ? i + block_length : width;
      tmp->y_end = (j + block_height < height) ? j + block_height : height;
      if (!ds->put(reinterpret_cast<uint64_t>(tmp))) {
        fprintf(stderr, "%s: ds_put failed\n", __func__);
        abort();
      }
    }
  }
  __sync_fetch_and_sub(&g_work_flag, 1);
}
