#include "include/array.h"
#include "include/console.h"
#include <stdlib.h>

#define null NULL

array_t array_init(uint16_t element_size)
{
    array_t result;
    result.element_size = element_size;
    result.capacity = ARRAY_START_SIZE;
    result.used = 0;
    result.data = malloc(result.capacity * result.element_size);
    return result;
}

static void array_ensure_capacity(array_t* array, size_t capacity)
{
    if (capacity <= array->capacity) return;
    array->capacity *= ARRAY_GROW_FACTOR;
    array->data = realloc(array->data, array->capacity * array->element_size);
    if (array->data == null) {
        log_fatal("Failed to reallocate array with new capacity of %d", array->capacity);
        exit(-3);
    }
}

void* array_append(array_t* array)
{
    array_ensure_capacity(array, array->used+1);
    void* slot = (char*)array->data + array->used * array->element_size;
    array->used++;
    return slot;
}

void* array_get(array_t* array, size_t index)
{
    void* slot = (char*)array->data + index * array->element_size;
    return slot;
}

uint32_t array_len(array_t* array) 
{
    return array->used;
}

void array_ensure_extra_capacity(array_t* array, uint32_t count)
{
    array_ensure_capacity(array, array->capacity + count);
}

void array_deinit(array_t* array)
{
    free(array->data);
}