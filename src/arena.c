#include "include/arena.h"
#include <Windows.h>
#include <stdio.h>

#define todo(thing) {printf("ERROR \"%s\" IS NOT IMPLEMENTED!", thing); exit(-2); }
#define null NULL

struct arena_marker_t {
    struct arena_marker_t* prev;
};

struct arena_block {
    struct arena_block* next;
    struct arena_marker_t* last_marker;
    size_t capacity;
    void* cur;
    // data
};

#define arena_start_of_data(block) ((char*)block + sizeof(struct arena_block))
#define arena_data_size 2097152

arena_t arena_init(void)
{
    arena_t result;
    // determine page size
    result.first = VirtualAlloc(null, 2097152, MEM_COMMIT | MEM_RESERVE , PAGE_READWRITE);
    if (result.first == null) {
        printf("ERROR in VirtualAlloc! requested_size: %d; Error: %lu\n", arena_data_size, GetLastError());
        exit(-1);
    }
    struct arena_block* block = result.first;
           block->capacity    = arena_data_size;
           block->last_marker = null;
           block->next        = null;
           block->cur         = arena_start_of_data(block);
    return result;
}

// size: requested size in bytes
void* arena_alloc(arena_t* arena, uint32_t size)
{
    uint32_t used = (size_t)arena->first->cur - (size_t)arena->first;
    uint32_t left = arena->first->capacity - used;
    if (left < size) {
        todo("alloc new page");
    }
    void* result = arena->first->cur;
    arena->first->cur = (char*)arena->first->cur + size;
    return result;
}

void arena_push_marker(arena_t* arena) 
{
    struct arena_marker_t* marker = arena_alloc(arena, sizeof(struct arena_marker_t));
    marker->prev              = arena->first->last_marker;
    arena->first->last_marker = marker;
    return;
}

void arena_pop_marker(arena_t* arena)
{
    struct arena_marker_t* marker = arena->first->last_marker;
    if (marker == null) {
        return;
    }
    arena->first->cur = marker;
    arena->first->last_marker = arena->first->last_marker->prev;
    return;
}

void arena_reset(arena_t* arena) 
{
    arena->first->cur = arena_start_of_data(arena->first);
    return;
}

void arena_deinit(arena_t* arena)
{
    if (arena->first->next != null) {
        todo("freeing other blocks");
    }
    bool ok = VirtualFree(arena->first, 0, MEM_RELEASE);
    if (!ok) {
        printf("ERROR failed to free arena; Error: %lu", GetLastError());
        exit(-2);
    }
}


#if 0
#define arena_alloc_offset ((size_t)arena.first->cur - (size_t)arena.first - 32)
static void test_arena()
{
    arena_t arena = arena_init();
    int i = 0;
    assert(arena_alloc_offset == 0);

    arena_alloc(&arena, 15 * sizeof(char));
    assert(arena_alloc_offset == 15);

    arena_alloc(&arena, 15 * sizeof(char));
    assert(arena_alloc_offset == 30);

    arena_push_marker(&arena);
        arena_alloc(&arena, 34);
        assert(arena_alloc_offset == 72);
    
        arena_push_marker(&arena);
            arena_alloc(&arena, 35);
            assert(arena_alloc_offset == 115);
        arena_pop_marker(&arena);

        assert(arena_alloc_offset == 72);
        arena_alloc(&arena, 34);
        assert(arena_alloc_offset == 106);
    
    arena_pop_marker(&arena);
    assert(arena_alloc_offset == 30);
 
    arena_reset(&arena);
    assert(arena_alloc_offset == 0);
    arena_alloc(&arena, 420);
    assert(arena_alloc_offset == 420);
    arena_deinit(&arena);
}
#endif