#pragma once
#include <stdint.h>
#include "array.h"

#ifndef HAS_ORIGINAL_ERRORS_ARRAY
extern array_t errors;
#endif

#define null NULL
#define for_to(init, upper_boundary) for(int (init) = 0; (init) < (upper_boundary); (init++))

#include <stdint.h>

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

typedef struct span_t {
    uint8_t file_id; // only 255 files for now
    uint32_t line;
    uint32_t col;
    uint32_t len;
} span_t;

typedef struct {
    log_level_e lvl;
    char* error;
    span_t error_loc;
    char* hint;
    span_t hint_loc;
} error_t;

inline void make_error_h(char* error, span_t error_loc, char* hint, span_t hint_loc)
{
    error_t* err = (error_t*)array_append(&errors);
    err->lvl = LOG_ERROR;
    err->error = error;
    err->error_loc = error_loc;
    err->hint = hint;
    err->hint_loc = hint_loc;
}

inline void make_error(char* error, span_t error_loc)
{
    error_t* err = (error_t*)array_append(&errors);
    err->lvl = LOG_ERROR;
    err->error = error;
    err->error_loc = error_loc;
    err->hint = null;
}