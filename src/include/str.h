#pragma once
#include <stdint.h>

typedef struct {
    char* data;
    uint16_t len;
} str_t;

inline str_t make_str(char* str, uint16_t len)
{
    str_t result;
    result.data = str; result.len = len;
    return result;
}

char* str_to_cstr(str_t* str);
// ACHTUNG: NICHT IM LEXER BENUTZEN!!!!
str_t str_from_char(char* source, uint16_t len);
str_t str_replace2_1(str_t str, char* replace, char with);
