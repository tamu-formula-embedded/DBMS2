/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 */
#ifndef EXP_ARR_H
#define EXP_ARR_H

#include <stdlib.h>

/*
 * A dynamic, pointer-based array that expands automatically
 * when accessed or written to out-of-bounds.
 *
 * Unlike a traditional vector, exp_arr does not track the number
 * of elements inserted — only the allocated capacity.
 * It is intended as a generic resizable buffer of void* slots.
 */
struct exp_arr {
    size_t  size;     // number of allocated slots (not the number of set items)
    void    **items;  // array of void* pointers
};

/*
 * Retrieves the pointer stored at index i.
 * If the index is out of bounds, the array expands to fit it.
 * New slots are initialized to NULL.
 * 
 * Parameters:
 *     arr - pointer to the expanding array
 *     i   - index to access
 *
 * Returns:
 *     The pointer stored at index `i` (may be NULL if not previously set).
 */
void *exp_arr_get(struct exp_arr *arr, size_t i);

/*
 * Sets the value at index i to v.
 * If the index is out of bounds, the array expands to fit it.
 * New slots are initialized to NULL except the slot i, which is set to v.
 *
 * Parameters:
 *     arr - pointer to the expanding array
 *     i   - index to write to
 *     v   - value to store (void*)
 */
void exp_arr_set(struct exp_arr *arr, size_t i, void* v);

#endif // EXP_ARR_H
