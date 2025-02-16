#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "str.h"

#define null NULL

typedef struct Map {
    struct Map* left;
    struct Map* right;
    struct Map* parent;
    u64 hash;
    void* value;
    bool is_red;
} Map;

void* map_get(Map* root, char* key, u32 len);
void map_set(Map* root, char* key, u32 len, void* value);
void* map_gets(Map* root, Str8 key);
void map_sets(Map* root, Str8 key, void* value);

void map_seth(Map* root, u64 hash, void* value);
void* map_geth(Map* root, u64 hash);

Map* map_get_at(Map* root, u32 index);
Map* map_next(Map* cur);