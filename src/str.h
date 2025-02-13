#pragma once
#include "misc.h"
#include <stdbool.h>

typedef struct {
    u16 len;
    char* data;
} Str8;

#define const_str(str) (Str8) {.data=str, .len=sizeof(str)}
#define null_str (Str8) {.len=0}

inline Str8 make_str(char* str, u16 len)
{
    Str8 result;
    result.data = str; result.len = len;
    return result;
}

char* str_to_cstr(Str8* str);
Str8 str_from_char(char* source, u16 len);
Str8 str_get_last_n(Str8* target, u32 n);
bool str_cmp(Str8* a , Str8* b);
bool str_cmp_c(Str8* a, char* b);
Str8 str_replace2_1(Str8 str, char* replace, char with);
void str_replace(Str8* str, char replace, char with);
