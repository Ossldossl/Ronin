#include "include/map.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST_MAP
#include <stdio.h>
uint64_t fnv1a(char* start, char* end)
{
    return rand() % 255+1;
}
#else

uint64_t fnv1a(char* start, char* end)
{
    const uint64_t magic_prime = 0x00000100000001b3;
    uint64_t hash = 0xcbf29ce484222325;
    for (; start <= end; start++) {
        hash = (hash ^ *start) * magic_prime;
    }
    return hash;
}
#endif

void* map_get(map_t* root, char* key, uint32_t len)
{
    if (root->parent != null) {
        while (root->parent) root = root->parent;
    }
    if (len == 0) {
        len = strlen(key);
    }
    uint64_t hash = fnv1a(key, key + len-1);
    map_t* cur = root;
    while (true) {
        if (cur == null || cur->hash == hash) break;
        cur = hash < cur->hash ? cur->left : cur->right;
    }
    return cur ? cur->value : null;
}

static void rotate_right(map_t* x)
{
    map_t* y = x->left;
    x->left = y->right;
    if (y->right) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent) {
        if (x == (x->parent)->right) {
            x->parent->right = y;
        }
        else {
            x->parent->left = y;
        }
    }
    y->right = x;
    x->parent = y;
}

static void rotate_left(map_t* x)
{
    map_t* y = x->right;
    if (y == null) return;
    x->right = y->left;
    if (y->left) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent) {
        if (x == (x->parent)->left) {
            x->parent->left = y;
        }
        else {
            x->parent->right = y;
        }
    }
    y->left = x;
    x->parent = y;
}

static void rebalance_tree(map_t* cur)
{
    cur->is_red = true;
    while ( (cur->parent) && (cur->parent->is_red)) {
        if (cur->parent->parent == null) break;
        if (cur->parent == cur->parent->parent->left) {
            map_t* uncle = cur->parent->parent->right;
            if (uncle == null) break;
            if (uncle->is_red) {
                // case 1: switch colors
                cur->parent->is_red = false; uncle->is_red = false;
                cur->parent->parent->is_red = true;
                cur = cur->parent->parent;
            } else {
                if (cur == cur->parent->right) {
                    // case 2: move cur up and rotate
                    cur = cur->parent;
                    rotate_left(cur);
                }
                else {
                // case 3
                    cur->parent->is_red = false;
                    cur->parent->parent->is_red = true;
                    rotate_right(cur->parent->parent);
                }
            }
        } else {
            map_t* uncle = cur->parent->parent->left;
            if (uncle == null) break;
            if (uncle->is_red) {
                // case 1: switch colors
                cur->parent->is_red = false; uncle->is_red = false;
                cur->parent->parent->is_red = true;
                cur = cur->parent->parent;
            } else {
                if (cur == cur->parent->left) {
                    // case 2: move cur up and rotate
                    cur = cur->parent;
                    rotate_left(cur);
                }
                else {
                // case 3
                    cur->parent->is_red = false;
                    cur->parent->parent->is_red = true;
                    rotate_right(cur->parent->parent);
                }
            }
        }
    }
}

void map_set(map_t* root, char* key, uint32_t len, void* value)
{
    if (root->parent != null) {
        while (root->parent) root = root->parent;
    }
    if (len == 0) {
        len = strlen(key);
    }
    uint64_t hash = fnv1a(key, key + len-1);
    map_t* cur = root;
    map_t** place_to_insert = null;
    while (true) {
        if (hash == cur->hash) {
            cur->value = value;
            return;
        } else if (cur->hash == 0) {
            // root
            cur->hash = hash; cur->value = value;
            return;
        }
        if (hash < cur->hash) {
            if (cur->left == null) {
                place_to_insert = &cur->left; break;
            }
            cur = cur->left;
        }
        else if (hash > cur->hash) {
            if (cur->right == null) {
                place_to_insert = &cur->right; break;
            }
            cur = cur->right;
        }
    }
    *place_to_insert = malloc(sizeof(map_t)); map_t* new = *place_to_insert;
    new->hash = hash; new->left = new->right = null; new->value = value; new->parent = cur;
    new->is_red = true;

    rebalance_tree(new);
}

void* map_gets(map_t* root, str_t key)
{
    return map_get(root, key.data, key.len);
}

void map_sets(map_t* root, str_t key, void* value)
{
    return map_set(root, key.data, key.len, value);
}

map_t* map_next(map_t* cur)
{
    if (cur->right) {
        cur = cur->right;
        while (cur->left) {
            cur = cur->left;
        }
        return cur;
    } else { // cur right and cur left are always null at this point
        while (cur->parent && cur == cur->parent->right) {
            cur = cur->parent;
        }
        // when we're left of parent
        //  on the next iteration we should return parent->right
        return cur->parent; 
    }
}

#ifdef TEST_MAP
int main(int argc, char** argv)
{
    map_t root = (map_t) {0};
    for (int i = 0; i < 150; i++) {
        map_set(&root, "a", 1, null);
    }

    map_t* cur = map_get_at(&root, 0);
    do {
        printf("%llu, ", cur->hash);
    } while ((cur = map_next(cur)));
    return 0;
}
#endif


// traverses tree from the first node to the node with the zero-based index 'index'
map_t* map_get_at(map_t* root, uint32_t index)
{
    while (root->parent) {
        root = root->parent;
    }
    // first find the first node
    if (root->hash == 0) return null; // when root is empty we have no entries
    map_t* cur = root;
    while (cur->left) {
        cur = cur->left;
    }   
    // then advance the node until the right node is found
    for (int i = 0; i < index; i++) {
        cur = map_next(cur);
        if (cur == null) break;
    }
    return cur;
}