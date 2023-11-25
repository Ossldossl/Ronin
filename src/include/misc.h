#pragma once
#include <stdint.h>
#include "array.h"

#ifndef HAS_ORIGINAL_ERRORS_ARRAY
extern array_t errors;
#endif

#define null NULL

typedef struct span_t {
    uint32_t line;
    uint32_t col;
    uint32_t len;
} span_t;

typedef struct {
    char* data;
    uint16_t len;
} str_slice_t;

typedef struct {
    char* error;
    span_t error_loc;
    char* hint;
    span_t hint_loc;
} error_t;

inline void make_error_h(char* error, span_t error_loc, char* hint, span_t hint_loc)
{
    error_t* err = (error_t*)array_append(&errors);
    err->error = error;
    err->error_loc = error_loc;
    err->hint = hint;
    err->hint_loc = hint_loc;
}

inline void make_error(char* error, span_t error_loc)
{
    error_t* err = (error_t*)array_append(&errors);
    err->error = error;
    err->error_loc = error_loc;
    err->hint = null;
}