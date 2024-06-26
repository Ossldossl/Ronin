#pragma once
#include <stdint.h>

#define ARRAY_GROW_FACTOR 1.5f
#define ARRAY_START_SIZE 2

#define for_array(array, type) _count = array_len((array));for(int i=0;i<_count;i++) {type*e = array_get((array),i);

typedef struct {
    uint16_t element_size;
    void* data;
    size_t used;
    size_t capacity;
} array_t;

array_t array_init(uint16_t element_size);
void* array_append(array_t* array);
void* array_pop(array_t* array);
void* array_get(array_t* array, size_t index);
uint32_t array_len(array_t* array);
void array_ensure_extra_capacity(array_t* array, uint32_t count); // stellt sicher, dass mindestens <count> slots frei sind
void array_deinit(array_t* array);