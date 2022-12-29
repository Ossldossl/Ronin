#include "include/misc.h"


arena_allocator_t* arena_new(int size_of_type, int capacity) 
{
    arena_allocator_t* allocator = malloc(sizeof(arena_allocator_t));
    allocator->capacity     = capacity;
    allocator->data         = malloc(capacity * size_of_type);
    allocator->index        = 0;
    allocator->size_of_type = size_of_type;
    return allocator;
}

void arena_destroy(arena_allocator_t* allocator) 
{
    free(allocator->data);
    free(allocator);
    return;
}

void* arena_alloc(arena_allocator_t* allocator) 
{
    if (allocator->index == allocator->capacity) {
        printf("reallocated more memory\n");
        allocator->capacity = allocator->capacity * 1.5;
        allocator->data = realloc(allocator->data, allocator->capacity * allocator->size_of_type);
    }
    void* result = ((char*)allocator->data) + allocator->index * allocator->size_of_type;
    allocator->index++;
    return result;
}
