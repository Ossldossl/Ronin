#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
#include "include/str.h"
#include "include/arena.h"

#define USE_SIMD

extern arena_t arena;

char* str_to_cstr(str_t str)
{
    char* result = arena_alloc(&arena, str.len+1);
    memcpy_s(result, str.len+1, str.data, str.len);
    result[str.len] = '\0';
    return result;
}

// ACHTUNG: NICHT IM LEXER BENUTZEN!!!!
str_t str_from_char(char* source, uint16_t len)
{
    str_t result;
    result.data = arena_alloc(&arena, len+1);
    result.len = len;
    memcpy_s(result.data, result.len +1, source, len);
    result.data[len] = 0;
    return result;
}

// replace 2 chars with 1 char
// TODO: SIMD
str_t str_replace2_1(str_t str, char* replace, char with)
{
    str_t result;
    result.len = 0;
    result.data = malloc(str.len+1);
    for (int i = 0; i < str.len; i++) {
        char c = str.data[i];
        char new_c = c;
        if (c == replace[0]) {
            if (i < str.len && str.data[++i] == replace[1]) {
                new_c = with;
            }
        }
        result.data[result.len++] = new_c;
    }
    result.data[result.len] = 0;
    return result;
}
