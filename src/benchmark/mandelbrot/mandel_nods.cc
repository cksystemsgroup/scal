#include <errno.h>
#include <getopt.h>
#include <gflags/gflags.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "benchmark/mandelbrot/bitmap.h"
#include "benchmark/mandelbrot/rgba.h"
#include "util/malloc.h"
#include "util/random.h"
#include "util/threadlocals.h"

DEFINE_uint64(prealloc_size, 65536 /* 256 MB */  / 2, "size in TLABs (most likely a page)");
DEFINE_bool(print_config, true, "print the Mandelbrot configuration");
DEFINE_double(x, 0.0, "x coordinate of image center");
DEFINE_double(y, 0.0, "y coordinate of image center");
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

struct package_t {
    int x_start;
    int x_end;
    int y_start;
    int y_end;
};

typedef struct __thread_arg {
    void *ds;
    pthread_barrier_t start_barrier;
    uint64_t global_start_time;
    uint64_t thread_id_cnt;
    struct bitmap *bm;
    double xmin;
    double xmax;
    double xrange;
    double ymin;
    double ymax;
    double yrange;
    int work_flag;
} thread_arg_t __attribute__ (( aligned(4096) ));

static __inline__ uint64_t get_utime (void);
static void worker (thread_arg_t *, uint64_t);
static uint32_t mandelbrot_point (double, double, int);
static uint32_t iteration_to_color (int, int);
static void * thread_func (void *);
static void compute_block (thread_arg_t *, int, int, int, int);

int 
main (int argc, const char **argv) {
    std::string usage("compute a Mandelbort image in parallel\n\n  Sample usage:\n\
    mandel -x -0.5 -y -0.5 -s 0.2\n\
    mandel -x -.38 -y -.665 -s .05 -m 100\n\
    mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000");
    google::SetUsageMessage(usage);
    google::ParseCommandLineFlags(&argc, (char***)&argv, true);

    pthread_t *threads = NULL;
    uint64_t num_threads;
    thread_arg_t targ;
    const char *outfile = (const char*)FLAGS_output.c_str();

    if (FLAGS_producers < 1) {
        fprintf (stderr, "atleast one producer is needed");
        return EXIT_FAILURE;
    }
    if (FLAGS_consumers < 1) {
        fprintf (stderr, "atleast one consumer is needed");
        return EXIT_FAILURE;
    }

    if (FLAGS_print_config) {
        printf("mandel: x=%lf y=%lf scale=%lf max=%lu width=%lu height=%lu \
block_width=%lu block_height=%lu producer=%lu consumer=%lu outfile=%s\n", 
                FLAGS_x, FLAGS_y, FLAGS_scale, FLAGS_max, FLAGS_width,
                FLAGS_height, FLAGS_block_length, FLAGS_block_height, 
                FLAGS_producers, FLAGS_consumers, FLAGS_output.c_str());
    }

    num_threads = FLAGS_producers + FLAGS_consumers;
    FLAGS_producers += FLAGS_consumers;
    FLAGS_consumers = 0;
    struct bitmap *bm = bitmap_create (FLAGS_width, FLAGS_height);
    bitmap_reset (bm, rgba_create (0, 0, 255, 0));

    targ.global_start_time = 0;
    targ.bm = bm;
    targ.thread_id_cnt = 0;
    targ.xmin = FLAGS_x - FLAGS_scale;
    targ.xmax = FLAGS_x + FLAGS_scale;
    targ.xrange = targ.xmax - targ.xmin;
    targ.ymin = FLAGS_y - FLAGS_scale;
    targ.ymax = FLAGS_y + FLAGS_scale;
    targ.yrange = targ.ymax - targ.ymin;
    targ.work_flag = FLAGS_producers;

    struct sched_param param;
    param.sched_priority = 40;
    if (sched_setscheduler(0, SCHED_RR, &param)) {
        fprintf(stderr, "WARNING: could not set RT priority, continue with normal priority\n");
    }

    uint64_t i;

    threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        perror ("malloc threads");
        return EXIT_FAILURE;
    }

    if (pthread_barrier_init (&targ.start_barrier, NULL, num_threads)) {
        fprintf (stderr, "unable to init start barrier");
        return EXIT_FAILURE;
    }

    for (i = 0; i < num_threads; i++) {
        pthread_create (&threads[i], NULL, thread_func, &targ);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join (threads[i], NULL);
    }

    uint64_t global_end_time = get_utime ();
    printf("%lu %lu %lu %lu\n",
        FLAGS_producers + FLAGS_consumers,
        FLAGS_producers,
        FLAGS_consumers,
        global_end_time - targ.global_start_time);

	if (FLAGS_generate_picture && !bitmap_save (bm, outfile)) {
		fprintf(stderr,"%s: couldn't write to %s: %s\n", __func__, outfile, strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

__inline__ uint64_t
get_utime () {
    struct timeval tv;
    uint64_t usecs;
    gettimeofday (&tv, NULL);
    usecs = tv.tv_usec + tv.tv_sec * 1000000;
    return usecs;
}


static void * thread_func (void *arg) {
    scal::tlalloc_init(FLAGS_prealloc_size, true /* touch pages */);
    threadlocals_init();
    int rc;
    uint64_t thread_id;
    thread_arg_t *targ = (thread_arg_t*)arg;
    thread_id = __sync_fetch_and_add (&targ->thread_id_cnt, 1);
    rc = pthread_barrier_wait (&targ->start_barrier);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        fprintf (stderr, "unable to wait on barrier");
        exit (EXIT_FAILURE);
    }
    if (targ->global_start_time == 0) {
        __sync_bool_compare_and_swap (&targ->global_start_time, 0, get_utime ());
    }
    worker(targ, thread_id);
    return NULL;
}

static void worker(thread_arg_t *targ, uint64_t thread_id) {
    int width = bitmap_width (targ->bm); 
    int height = bitmap_height (targ->bm);
    int i, j;
    int block_height = FLAGS_block_height;
    int block_length = FLAGS_block_length;
    int height_step = block_height * FLAGS_producers;
    int length_step = block_length;
    //package_t *tmp = (package_t*) malloc(1 * sizeof(package_t));
    for (j = block_height * thread_id; j < height; j += height_step) {
        for (i = 0; i < width; i += length_step) {
            /*
            tmp = (package_t*) malloc(1 * sizeof(package_t)); 
            tmp->x_start = i;
            tmp->y_start = j;
            tmp->x_end = (i + block_length < width) ? i + block_length : width;
            tmp->y_end = (j + block_height < height) ? j + block_height : height;
            compute_block (targ, tmp->x_start, tmp->y_start, tmp->x_end, tmp->y_end);
            */    
            compute_block(targ,
                          i,
                          j,
                          (i + block_length < width) ? i + block_length : width,
                          (j + block_height < height) ? j + block_height : height);
        }
    }
}

static void
compute_block (thread_arg_t *targ, int x_start, int y_start, int x_end, int y_end) {
    int i;
    int j;
    double x;
    double y;
    uint32_t color;
    for (i = x_start; i < x_end; i++) {
        for (j = y_start; j < y_end; j++) {
            x =  targ->xmin + i * targ->xrange / bitmap_width (targ->bm);
            y =  targ->ymin + j * targ->yrange / bitmap_height (targ->bm);
            color = mandelbrot_point (x, y, FLAGS_max);
            bitmap_set (targ->bm, i, j, color);
        }
    }
}

static uint32_t
mandelbrot_point (double x, double y, int max) {
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
	return iteration_to_color (i, max);
}

static uint32_t
iteration_to_color (int i, int max) {
	int gray = 255 * i / max;
	return rgba_create (gray, gray, gray, 0);
}
