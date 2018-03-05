#pragma once

#define CACHE_LINE_SIZE 1024
#define ALIGN_CACHE __attribute__ ((aligned (CACHE_LINE_SIZE)))

#ifdef __ANDROID_API__
#include <android/log.h>
#define LOG(format, ...) __android_log_print(ANDROID_LOG_DEBUG, "libspectre", format, __VA_ARGS__)
#else
#include <stdio.h>
#define LOG(format, ...) fprintf(stderr, format "\n", __VA_ARGS__)
#endif
