#pragma once
#include <stdint.h>

typedef struct {
    uint32_t line;
    uint32_t col;
    uint32_t len;
} span_t;

typedef struct {
    span_t loc;
    char* error;
    char* hint;
} error_t;