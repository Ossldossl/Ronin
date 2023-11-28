#include <stdlib.h>
#include <string.h>

#include "include/str.h"
#include "include/arena.h"

extern arena_t arena;

char* str_to_cstr(str_t str)
{
    char* result = arena_alloc(&arena, str.len+1);
    memcpy_s(result, str.len+1, str.data, str.len);
    result[str.len] = '\0';
    return result;
}