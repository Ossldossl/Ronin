#include <stdlib.h>
#include <string.h>
#include "str.h"
#include "console.h"
#include "arena.h"

//#define USE_SIMD

extern Arena arena;

Str8 str_get_last_n(Str8* target, u32 n)
{
    if (n > target->len) {
        log_fatal("Can't get last %d chars of %d long string!", n, target->len);
        exit(-1);
    }
    Str8 result;
    result.data = target->data + (target->len-n);
    result.len = n;
    return result;
}

// expects both strings to be of the same length (excluding the null char)
bool str_cmp_c(Str8* a, char* b)
{
    char* ad = a->data; 
    char* e = ad + a->len;
    while (ad < e) {
        if (*ad++ != *e) return false;
    }
    return true;
}

bool str_cmp(Str8* a, Str8* b)
{
    if (a->len != b->len) return false;
    char* ad = a->data; char* bd = b->data;
    char* ae = a->data + a->len;
    while (ad < ae) {
        if (*ad++ != *bd++) return false;
    }
    return true;
}

char* str_to_cstr(Str8* str)
{
    char* result = arena_alloc(&arena, (str->len+1));
    memcpy_s(result, str->len+1, str->data, str->len);
    result[str->len] = '\0';
    return result;
}

Str8 str_from_char(char* source, uint16_t len)
{
    Str8 result;
    result.data = malloc(len+1);
    result.len = len;
    memcpy_s(result.data, result.len +1, source, len);
    result.data[len] = 0;
    return result;
}

// replace 2 chars with 1 char
// TODO: SIMD
Str8 str_replace2_1(Str8 str, char* replace, char with)
{
    Str8 result;
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

void str_replace(Str8* str, char replace, char with)
{
    char* ds = str->data; char* de = str->data + str->len;
    while (ds++ <= de) {
        if (*ds == replace) {
            *ds = with;
        }
    }
}