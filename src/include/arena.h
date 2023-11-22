#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    struct arena_block* first;
    uint32_t page_size;
} arena_t;

arena_t arena_init(void);
void arena_push_marker(arena_t* arena);
void* arena_alloc(arena_t* arena, uint32_t size);
void arena_pop_marker(arena_t* arena);
void arena_reset(arena_t* arena);
void arena_deinit(arena_t* arena);