#pragma once

#define DEBUG 

#include <stdio.h>

#include "types.h"
#include "util.h"

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

/* logging */
#define LOG_CRIT(fmt, ...) \
    fprintf(stderr, "[CRIT] (%s:%d) " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, "[ERROR] (%s:%d) " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)

#ifdef DEBUG
    #define LOG_DEBUG(fmt, ...) \
        fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif
