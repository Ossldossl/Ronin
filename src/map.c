#include "misc.h"
#include "map.h"

#include <stdlib.h>
#include <string.h>

#ifdef TEST_MAP
#include <stdio.h>
u64 fnv1a(char* start, char* end)
{
    return rand() % 255+1;
}
#else
u64 fnv1a(char* start, char* end)
{
    const u64 magic_prime = 0x00000100000001b3;
    u64 hash = 0xcbf29ce484222325;
    for (; start <= end; start++) {
        hash = (hash ^ *start) * magic_prime;
    }
    return hash;
}
#endif

void* map_get(Map* root, char* key, u32 len)
{
    if (root->parent != null) {
        while (root->parent) root = root->parent;
    }
    if (len == 0) {
        len = strlen(key);
    }
    u64 hash = fnv1a(key, key + len-1);
    Map* cur = root;
    while (true) {
        if (cur == null || cur->hash == hash) break;
        cur = hash < cur->hash ? cur->left : cur->right;
    }
    return cur ? cur->value : null;
}

static void rotate_right(Map* x)
{
    Map* y = x->left;
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

static void rotate_left(Map* x)
{
    Map* y = x->right;
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

static void rebalance_tree(Map* cur)
{
    cur->is_red = true;
    while ( (cur->parent) && (cur->parent->is_red)) {
        if (cur->parent->parent == null) break;
        if (cur->parent == cur->parent->parent->left) {
            Map* uncle = cur->parent->parent->right;
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
            Map* uncle = cur->parent->parent->left;
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

void map_set(Map* root, char* key, u32 len, void* value)
{
    if (root->parent != null) {
        while (root->parent) root = root->parent;
    }
    if (len == 0) {
        len = strlen(key);
    }
    u64 hash = fnv1a(key, key + len-1);
    Map* cur = root;
    Map** place_to_insert = null;
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
    *place_to_insert = malloc(sizeof(Map)); Map* new = *place_to_insert;
    new->hash = hash; new->left = new->right = null; new->value = value; new->parent = cur;
    new->is_red = true;

    rebalance_tree(new);
}

void* map_gets(Map* root, Str8 key)
{
    return map_get(root, key.data, key.len);
}

void map_sets(Map* root, Str8 key, void* value)
{
    return map_set(root, key.data, key.len, value);
}

Map* map_next(Map* cur)
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
    Map root = (Map) {0};
    for (int i = 0; i < 150; i++) {
        map_set(&root, "a", 1, null);
    }

    Map* cur = map_get_at(&root, 0);
    do {
        printf("%llu, ", cur->hash);
    } while ((cur = map_next(cur)));
    return 0;
}
#endif


// traverses tree from the first node to the node with the zero-based index 'index'
Map* map_get_at(Map* root, u32 index)
{
    while (root->parent) {
        root = root->parent;
    }
    // first find the first node
    if (root->hash == 0) return null; // when root is empty we have no entries
    Map* cur = root;
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