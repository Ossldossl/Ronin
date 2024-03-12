#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "include/misc.h"
#include "include/allocators.h"
#include "include/console.h"

#include <sanitizer/asan_interface.h>

static u64 alloc_counter = 0;

void* arena_alloc(arena_t* a, u32 size)
{
//    log_debug("arena_alloc with size %u", size);
    alloc_counter++;
    arena_body* bucket = a->buckets[a->bucket_count-1];
    void* new_cur = INC_PTR(bucket->cur, size);
    if ((u64)new_cur - (u64)bucket - 24 >= ARENA_SIZE) {
        // allocation too big, make new bucket
        log_debug("%lu: New bucket after %u bytes and %d allocs", alloc_counter, (u64)bucket->cur - (u64)bucket - 24, alloc_counter);
        alloc_counter = 0;

        a->bucket_count++; HeapReAlloc(a->heap, HEAP_ZERO_MEMORY, a->buckets, a->bucket_count * sizeof(arena_body*));
        if (a->buckets == null) {
            log_fatal("Failed to realloc bucket ptr array!"); exit(-1);
        }
        arena_body* body = VirtualAlloc(a->buckets[a->bucket_count-1], ARENA_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (body == null) {
            log_fatal("Failed to allocate new body!: %lu", GetLastError()); exit(-1);
        }

        body->cur = ARENA_DATA(body); body->last_alloc_size = 0; body->last = null;
        a->buckets[a->bucket_count-1] = body;
        new_cur = INC_PTR(bucket->cur, size);
        bucket = body;
    }
//    ASAN_UNPOISON_MEMORY_REGION(bucket->cur, size);
    void* result = bucket->cur;
    bucket->last_alloc_size = size;
    bucket->cur = new_cur; 
    return result;
}

void arena_free_last(arena_t* arena)
{
    arena_body* bucket = arena->buckets[arena->bucket_count-1];
    bucket->cur = INC_PTR(bucket->cur, -(i64)bucket->last_alloc_size);
   // ASAN_POISON_MEMORY_REGION(bucket->cur, bucket->last_alloc_size);
    bucket->last_alloc_size = 0;
}

void arena_begin_section(arena_t* arena)
{
    arena_section* sec = arena_alloc(arena, sizeof(arena_section));
    arena_body* bucket = arena->buckets[arena->bucket_count-1];
    sec->prev = bucket->last;
    bucket->last = sec;
}

void arena_end_section(arena_t* arena)
{
    arena_body* bucket = arena->buckets[arena->bucket_count-1];
    arena_section* prev = bucket->last->prev;
    ASAN_POISON_MEMORY_REGION(bucket->last, (u64)bucket->cur - (u64)bucket->last);
    bucket->last = prev;
    if (bucket->last) bucket->cur = bucket->last; // free memory up until the section
}

void* arena_get_cur(arena_t* arena)
{
    return arena->buckets[arena->bucket_count-1]->cur;
}

arena_t make_arena()
{
    arena_t result;
    result.bucket_count = 1;

    result.heap = GetProcessHeap();    
    result.buckets = HeapAlloc(result.heap, HEAP_ZERO_MEMORY, sizeof(arena_body*));
    if (result.buckets == null) {
        log_fatal("Failed to allocate buckets!"); exit(-1);
    }

    arena_body* first = VirtualAlloc(null, ARENA_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (first == null) {
        log_fatal("Failed to allocate arena data: %lu", GetLastError()); exit(-1);
    }
    first->cur = ARENA_DATA(first); first->last = null; first->last_alloc_size = 0;
    result.buckets[0] = first;
    return result;
}

void destroy_arena(arena_t* arena)
{
    for_to(i, arena->bucket_count) {
        VirtualFree(arena->buckets[i], ARENA_SIZE, MEM_RELEASE);
    }
}