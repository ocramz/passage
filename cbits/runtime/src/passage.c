#include "passage.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

// An arbitrary limit to the number of seeds we might have
#define MAX_SEEDS 1024

unsigned long number_of_samples;
unsigned long steps_per_sample;
unsigned long warm_up_steps;
static unsigned long seeds[MAX_SEEDS];
unsigned long num_threads;
int have_seed;


/* State for the random number generator */

#ifdef __USE_MERSENNE
unsigned long mag01[2];
unsigned long mt[N];      /* the array for the state vector  */
int mti;                  /* mti==N+1 means mt[N] is not initialized */
#else
unsigned int rx, ry, rz, rw, rc;
#endif







/* Make up a random seed */
static
unsigned long getSeed() {
  struct timeval tv;
  unsigned long seed;
  int problem = 1;
  int fd;

  if ((fd = open("/dev/urandom", O_RDONLY)) >= 0 ||
      (fd = open("/dev/random", O_RDONLY)) >= 0) {
          ssize_t n = read(fd, &seed, sizeof(seed));
          if (n == sizeof(seed)) problem = 0;
          close(fd);
  }
  if (problem) {
    gettimeofday(&tv, NULL);
    seed = (getpid() << 16) ^ tv.tv_sec ^ tv.tv_usec;
  }

  return seed;
}

static
int get_num(unsigned long *out) {
  char *end;
  unsigned long r;
  r = strtoul(optarg, &end, 10);
  if (*end == '\0' && optarg != '\0') { *out = r; return 1; }
  return 0;
}

/* Generated by DSL */
extern void sampler(void);
extern void set_defaults(void);
extern void init_vars(void);

int main(int argc, char *argv[]) {
  int r, i;

  set_defaults();

  do {
    r = getopt(argc, argv, "n:i:w:s:h");
    if (r == -1) break;

    switch (r) {
      case 'n': if (!get_num(&number_of_samples)) goto err; break;
      case 'i': if (!get_num(&steps_per_sample))  goto err; break;
      case 'w': if (!get_num(&warm_up_steps))     goto err; break;
      case 's':
        if (have_seed >= MAX_SEEDS) goto err;
        if (!get_num(&seeds[have_seed])) goto err; ++have_seed;
        break;

      default: goto err;
   }

  } while (1);
  if (optind != argc) goto err;

  if (num_threads >= MAX_SEEDS) {
    fprintf(stderr, "Too many threads (limit %d)\n", MAX_SEEDS);
    return 2;
  }

  for (; have_seed < num_threads; ++have_seed)
    seeds[have_seed] = getSeed();

  for (i = 0; i < have_seed; ++i)
    fprintf(stderr, "Seed[%d]:     %lu\n", i, seeds[i]);
  fprintf(stderr, "Samples:      %lu\n", number_of_samples);
  fprintf(stderr, "Steps/sample: %lu\n", steps_per_sample);
  fprintf(stderr, "Warm-up:      %lu\n", warm_up_steps);
  fprintf(stderr, "Threads:      %lu\n", num_threads);

  #pragma omp parallel num_threads(num_threads)
  {
    init_genrand(seeds[omp_get_thread_num()]);

    #pragma omp single
    init_vars();

    sampler();
  }

  return 0;

err:
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  -n NUM\tNumber of samples to generate.\n");
  fprintf(stderr, "  -i NUM\tNumber of iterations per sample.\n");
  fprintf(stderr, "  -w NUM\tNumber of iterations for calibrating sampling window.\n");
  fprintf(stderr, "  -s NUM\tFixed seed(s) of randomness.\n");
  fprintf(stderr, "  -h    \tThis help.\n");
  return 1;
}



void crash_out_of_bounds(int line) {
  fprintf(stderr, "Array index out of bounds on line %d\n", line);
  exit(1);
}

