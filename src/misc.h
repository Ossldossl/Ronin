#pragma once
#include <stdint.h>
#include <stdbool.h>

#define null NULL
#define for_to(init, upper_boundary) for(int (init) = 0; (init) < (upper_boundary); (init++))

typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
} log_level_e;

