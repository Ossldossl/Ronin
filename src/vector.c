#include "include/misc.h"

vector_t* vector_new(void)
{
    vector_t* vector = malloc(sizeof(vector_t));
    CHECK_ALLOCATION(vector);
    vector->size = 2;
    vector->used = 0;
    vector->data = malloc(vector->size * sizeof(void*));
    CHECK_ALLOCATION(vector->data)
    return vector;
}

void vector_push(vector_t* vector, void* element)
{
    if (vector->used == vector->size)
    {
        vector->size *= 2;
        vector->data = realloc(vector->data, vector->size * sizeof(void*));
    }
    vector->data[vector->used] = element;
    vector->used++;
}

void* vector_pop(vector_t* vector)
{
    if (vector->used == 0)
    {
        return null;
    }
    void* result = vector->data[vector->used - 1];
    vector->used--;
    return result;
}

void vector_free(vector_t* vector)
{
    for (int i = 0; i < vector->used; i++)
    {
        free(vector->data[i]);
    }
    free(vector->data);
    free(vector);
}