#pragma once
#include "misc.h"

#define ARRAY_GROW_FACTOR 1.5f
#define ARRAY_START_SIZE 2

#define for_array(array, type) _count = array_len((array));for(int i=0;i<_count;i++) {type* e = array_get((array),i);
#define ArrayOf(type) Array

typedef struct {
    u16 element_size;
    void* data;
    u32 used;
    u32 capacity;
} Array;

Array array_init(u16 element_size);
void* array_append(Array* array);
void* array_pop(Array* array);
void* array_get(Array* array, size_t index);
uint32_t array_len(Array* array);
void array_ensure_extra_capacity(Array* array, u32 count); // stellt sicher, dass mindestens <count> slots frei sind
void array_deinit(Array* array);