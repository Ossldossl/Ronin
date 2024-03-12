#pragma once
#include <stdint.h>
#include "misc.h"

#define ARENA_SIZE UINT32_MAX
#define ARENA_DATA(body) (((char*)body + sizeof(arena_body)))
#define INC_PTR(ptr, inc) (((char*)ptr) + (inc))
#define ALLOC(alloc, size) (alloc)->allocate((alloc), (size))
#define FREE(alloc, ptr) (alloc)->free((alloc), (ptr))

typedef struct allocator allocator;
struct allocator {
    void (*free)(allocator* alloc, void* ptr);
    void* (*allocate)(allocator* alloc, u32 size);
};

typedef struct arena_section arena_section;

struct arena_section {
    arena_section* prev;
};

typedef struct arena_body arena_body;
struct arena_body {
    void* cur;
    u32 last_alloc_size;
    arena_section* last;
    // 2mb memory;
};

typedef struct {
    arena_body** buckets;
    u32 bucket_count; 
    void* heap;
} arena_t;

arena_t make_arena();
void arena_begin_section(arena_t* arena);
void arena_end_section(arena_t* arena);
void* arena_alloc(arena_t* arena, u32 size);
void arena_free_last(arena_t* arena);
void* arena_get_cur(arena_t* arena);
void destroy_arena(arena_t* arena);