#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "str.h"

#define null NULL

typedef struct map_t {
    struct map_t* left;
    struct map_t* right;
    struct map_t* parent;
    uint64_t hash;
    void* value;
    bool is_red;
} map_t;

void* map_get(map_t* root, char* key, uint32_t len);
void map_set(map_t* root, char* key, uint32_t len, void* value);
void* map_gets(map_t* root, str_t key);
void map_sets(map_t* root, str_t key, void* value);

map_t* map_get_at(map_t* root, uint32_t index);
map_t* map_next(map_t* cur);