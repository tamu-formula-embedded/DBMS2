#include "exp_arr.h"

#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void *exp_arr_get(struct exp_arr *arr, size_t i)
{
    if (i >= arr->size)
    {
        size_t new_size = MAX(i + 1, 2 * arr->size);
        void **new_items = calloc(new_size, sizeof(void*));  // calloc zero-inits

        if (arr->items)
            memcpy(new_items, arr->items, arr->size * sizeof(void*));

        free(arr->items);
        arr->items = new_items;
        arr->size = new_size;
    }
    return arr->items[i];
}

void exp_arr_set(struct exp_arr *arr, size_t i, void* v)
{
    if (i >= arr->size)
    {
        size_t new_size = MAX(i + 1, 2 * arr->size);
        void **new_items = calloc(new_size, sizeof(void*));  // calloc zero-inits

        if (arr->items)
            memcpy(new_items, arr->items, arr->size * sizeof(void*));

        free(arr->items);
        arr->items = new_items;
        arr->size = new_size;
    }
    arr->items[i] = v;
}