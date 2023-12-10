#include "include/map.h"

#include <stdlib.h>
#include <string.h>

uint64_t fnv1a(char* start, char* end)
{
    const uint64_t magic_prime = 0x00000100000001b3;
    uint64_t hash = 0xcbf29ce484222325;
    for (; start <= end; start++) {
        hash = (hash ^ *start) * magic_prime;
    }
    return hash;
}

void* map_get(map_t* root, char* key, uint32_t len)
{
    if (len == 0) {
        len = strlen(key);
    }
    uint64_t hash = fnv1a(key, key + len);
    map_t* cur = root;
    while (true) {
        if (cur == null || cur->hash == hash) break;
        else if (hash < cur->hash) cur = cur->left;
        else cur = cur->right;
    }
    if (cur) return cur->value;
    else return null;
}

// FIXME: No check if old_child is really a child of parent
static void replace_child(map_t* parent, map_t* old_child, map_t* new_child)
{
    free(old_child->value);
    free(old_child);
    if (parent->left == old_child) {
        parent->left = new_child;
    } else {
        parent->right = new_child;
    }
    if (new_child) {
        new_child->parent = parent;
    }
}

static void rotate_right(map_t* node)
{
    map_t* parent = node->parent;
    map_t* left_child = node->left;
    node->left = left_child->right;
    if (left_child->right) {
        left_child->right->parent = node;
    }
    left_child->right = node;
    node->parent = left_child;
    replace_child(parent, node, left_child);
}

static void rotate_left(map_t* node)
{
    map_t* parent = node->parent; map_t* right_child = node->right;

    node->right = right_child->left;
    if (right_child->left) {
        right_child->left->parent = node;
    }
    right_child->left = node;
    node->parent = right_child;
    replace_child(parent, node, right_child);
}

static void rebalance_tree(map_t* cur)
{
    map_t* parent = cur->parent;
    if (parent == null) { cur->is_red = false; return; }
    if (!parent->is_red) return;

    map_t* gp = parent->parent;
    if (gp == null) {
        // case 2: parent is root
        parent->is_red = false; return;
    }
    bool uncle_is_left = parent->hash > gp->hash;
    map_t* uncle = uncle_is_left ? gp->left : gp->right;
    if (uncle && uncle->is_red) {
        // case 3: uncle is red
        parent->is_red = false; gp->is_red = true; uncle->is_red = false;
        return rebalance_tree(gp);
    }

    if (!uncle_is_left) {
        if (cur == parent->right) {
            rotate_left(parent); parent = cur;
        }
        rotate_right(gp);
        parent->is_red = false;
        gp->is_red = true;
        return;
    }

    // parent is right child of gp
    if (cur == parent->left) {
        rotate_right(parent);
        parent = cur;
    }
    rotate_left(gp);
    parent->is_red = false; gp->is_red = true;
    return;
}

void map_set(map_t* root, char* key, uint32_t len, void* value)
{
    if (len == 0) {
        len = strlen(key);
    }
    uint64_t hash = fnv1a(key, key + len);
    map_t* cur = root;
    map_t** place_to_insert = null;
    while (true) {
        if (hash == cur->hash) {
            cur->value = value;
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