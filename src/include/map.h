#pragma once
#include <stdbool.h>
#include <stdint.h>

#define null NULL

typedef struct map_t {
    struct map_t* left;
    struct map_t* right;
    struct map_t* parent;
    uint64_t hash;
    void* value;
    bool is_red;
} map_t;

bool map_key_exists(map_t* root, char* key, uint32_t len);
void* map_get(map_t* root, char* key, uint32_t len);
void map_set(map_t* root, char* key, uint32_t len, void* value);
