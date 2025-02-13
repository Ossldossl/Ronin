#include "array.h"
#include "console.h"
#include <stdlib.h>

#define null NULL

Array array_init(u16 element_size)
{
    Array result;
    result.element_size = element_size;
    result.capacity = ARRAY_START_SIZE;
    result.used = 0;
    result.data = malloc(result.capacity * result.element_size);
    return result;
}

static void array_ensure_capacity(Array* array, size_t capacity)
{
    if (capacity <= array->capacity) return;
    array->capacity *= ARRAY_GROW_FACTOR;
    array->data = realloc(array->data, array->capacity * array->element_size);
    if (array->data == null) {
        log_fatal("Failed to reallocate array with new capacity of %d", array->capacity);
        exit(-3);
    }
}

void* array_append(Array* array)
{
    array_ensure_capacity(array, array->used+1);
    void* slot = (char*)array->data + array->used * array->element_size;
    array->used++;
    return slot;
}

void* array_pop(Array* array)
{
    return array_get(array, --array->used);
}

void* array_get(Array* array, size_t index)
{
    void* slot = (char*)array->data + index * array->element_size;
    return slot;
}

u32 array_len(Array* array) 
{
    return array->used;
}

void array_ensure_extra_capacity(Array* array, u32 count)
{
    array_ensure_capacity(array, array->capacity + count);
}

void array_deinit(Array* array)
{
    free(array->data);
}