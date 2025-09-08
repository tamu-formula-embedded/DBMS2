#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include <stddef.h>

// Lookup Table Entry mapping voltage to temperature
typedef struct{
    float key;
    float value;
} LTE;

// Builds a LUT from parallel arrays, copies up to max_count, then sorts.
void lut_build(LTE* out_entries,
                 const float* keys,
                 const float* values,
                 size_t count);

// Interpolates between two entries to find the closest value at a given key
float lut_interpolate(const LTE* entries, size_t count, float key);

#endif // LOOKUP_TABLE_H
