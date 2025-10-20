#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include <stddef.h>
#include <stdlib.h>

typedef struct
{
    float key;
    float value;
} LTE;

/**
 * Function to build a lookup table from arrays of keys and values.
 * Copies up to count entries from the input arrays, then sorts.
 */
void lut_build(LTE* out_entries, const float* keys, const float* values, size_t count);

/**
 * Function to interpolate between two entries to find the value at a given key
 * returns interpolated value, used to see which entry is closer
 */
float lut_interpolate(const LTE* entries, size_t count, float key);

#endif
