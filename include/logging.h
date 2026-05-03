#pragma once

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

/* logging */
#define PANIC(fmt, ...) do { \
    int _err = errno; \
    char _err_buf[256] = {0}; \
    strerror_r(_err, _err_buf, sizeof(_err_buf)); \
    fprintf(stderr, "[CRIT] (%s:%d) " fmt " | Error: %d (%s)\n", \
            __FILENAME__, __LINE__, ##__VA_ARGS__, _err, _err_buf); \
    abort(); \
} while (0)

#define LOG_ERROR(fmt, ...) do { \
    int _err = errno; \
    char _err_buf[256] = {0}; \
    strerror_r(_err, _err_buf, sizeof(_err_buf)); \
    fprintf(stderr, "[ERROR] (%s:%d) " fmt " | Error: %d (%s)\n", \
            __FILENAME__, __LINE__, ##__VA_ARGS__, _err, _err_buf); \
} while (0)

#define LOG_ERROR_CODE(err_code, fmt, ...) do { \
    int _err = (err_code); \
    /* io_uring 등에서 오는 음수 에러 코드를 양수로 변환하여 strerror_r에 전달 */ \
    int _abs_err = (_err < 0) ? -_err : _err; \
    char _err_buf[256] = {0}; \
    char *_err_msg = strerror_r(_abs_err, _err_buf, sizeof(_err_buf)); \
    fprintf(stderr, "[ERROR] (%s:%d) " fmt " | Error: %d (%s)\n", \
            __FILENAME__, __LINE__, ##__VA_ARGS__, _abs_err, _err_msg); \
} while (0)

#define LOG_ERROR_NO_STRERROR(fmt, ...) \
    fprintf(stdout, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)

#ifndef NDEBUG
    #define LOG_DEBUG(fmt, ...) \
        fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif
