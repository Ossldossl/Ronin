#pragma once
#include <stdint.h>
#include "misc.h"

#define ARENA_SIZE UINT32_MAX
#define ARENA_DATA(body) (((char*)body + sizeof(ArenaBody)))
#define INC_PTR(ptr, inc) (((char*)ptr) + (inc))
#define ALLOC(alloc, size) (alloc)->allocate((alloc), (size))
#define FREE(alloc, ptr) (alloc)->free((alloc), (ptr))

typedef struct Allocator Allocator;
struct Allocator {
    void (*free)(Allocator* alloc, void* ptr);
    void* (*allocate)(Allocator* alloc, u32 size);
};

typedef struct ArenaSection ArenaSection;

struct ArenaSection {
    ArenaSection* prev;
};

typedef struct ArenaBody ArenaBody;
struct ArenaBody {
    void* cur;
    u32 last_alloc_size;
    ArenaSection* last;
    // 2mb memory;
};

typedef struct {
    ArenaBody** buckets;
    u32 bucket_count; 
    void* heap;
} Arena;

Arena make_arena();
void arena_begin_section(Arena* arena);
void arena_end_section(Arena* arena);
void* arena_alloc(Arena* arena, u32 size);
void arena_free_last(Arena* arena);
void* arena_get_cur(Arena* arena);
void destroy_arena(Arena* arena);