#pragma once
#include <stdint.h>
#include <stdbool.h>

struct arena_marker_t {
    struct arena_marker_t* prev;
};

struct arena_block {
    struct arena_block* next;
    struct arena_marker_t* last_marker;
    size_t capacity;
    uint32_t last_alloc_size;
    void* cur;
    // data
};

typedef struct {
    struct arena_block* first;
    uint32_t page_size;
} arena_t;

arena_t arena_init(void);
void arena_push_marker(arena_t* arena);
void* arena_alloc(arena_t* arena, uint32_t size);
void arena_free_last(arena_t* arena);
void arena_pop_marker(arena_t* arena);
void arena_reset(arena_t* arena);
void arena_deinit(arena_t* arena);