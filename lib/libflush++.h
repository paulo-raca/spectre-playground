#pragma once

#include "common.h"
#include <libflush.h>

#include <sched.h>
#include <algorithm>

class LibFlush {
public:
    libflush_session_t* session;
    int cache_hit_threshold;

    inline LibFlush(size_t counter_thread_cpu = 0) {
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

    inline void* to_physical(const void* ptr) {
        return (void*) libflush_get_physical_address(session, (uintptr_t)ptr);
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

        uint8_t* flush_benchmark = new uint8_t[2*CACHE_LINE_SIZE];

        for (int i=0; i<test_count; i++) {
            sched_yield();
            flush(flush_benchmark + CACHE_LINE_SIZE);
            miss_times[i] = access_time(flush_benchmark + CACHE_LINE_SIZE);
            hit_times[i]  = access_time(flush_benchmark + CACHE_LINE_SIZE);
        }

        delete[] flush_benchmark;

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
