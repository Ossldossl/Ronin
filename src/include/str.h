#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t len;
    char* data;
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
str_t str_get_last_n(str_t* target, uint32_t n);
bool str_cmp(str_t* a , str_t* b);
bool str_cmp_c(str_t* a, char* b);
str_t str_replace2_1(str_t str, char* replace, char with);
void str_replace(str_t* str, char replace, char with);
