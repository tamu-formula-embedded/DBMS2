/** 
 * 
 * Distributed BMS      Lookup Table Module 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Cam Stone <cameron28202@tamu.edu>
 */
#include "lut.h"

// forward declaration
static int cmp_lte(const void* a, const void* b);

/**
 * Function to build a lookup table from arrays of keys and values.
 * Copies up to count entries from the input arrays, then sorts.
 */
void lut_build(LTE* out_entries, const float* keys, const float* values, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        out_entries[i].key = keys[i];
        out_entries[i].value = values[i];
    }

    qsort(out_entries, count, sizeof(LTE), cmp_lte);
}

/**
 * Helper function for binary search
 */
static int cmp_lte(const void* a, const void* b)
{
    const LTE* A = (const LTE*)a;
    const LTE* B = (const LTE*)b;
    return (A->key > B->key) - (A->key < B->key);
}

/**
 * Internal helper: find indices [left,right] such that
 * entries[left].key <= key <= entries[right].key
 */
static void lut_find_bracket(const LTE* entries, size_t count, float key, int* left, int* right)
{
    if (count == 0)
    {
        if (left) *left = -1;
        if (right) *right = -1;
        return;
    }

    // the list is sorted. if we are out of bounds, return the first or last entry
    if (key <= entries[0].key)
    {
        if (left) *left = 0;
        if (right) *right = 0;
        return;
    }
    if (key >= entries[count - 1].key)
    {
        if (left) *left = (int)count - 1;
        if (right) *right = (int)count - 1;
        return;
    }

    // binary search
    int low = 0;
    int high = (int)count - 1;
    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        float midval = entries[mid].key;
        if (midval == key)
        {
            if (left) *left = mid;
            if (right) *right = mid;
            return;
        }
        else if (midval < key)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    if (left) *left = high;
    if (right) *right = low;
}

/**
 * Function to interpolate between two entries to find the value at a given key
 * returns interpolated value, used to see which entry is closer
 */
float lut_interpolate(const LTE* entries, size_t count, float key)
{
    if (count == 0)
    {
        return 0.0f;
    }

    int left = -1, right = -1;
    lut_find_bracket(entries, count, key, &left, &right);
    if (left == -1)
    {
        return 0.0f;
    }
    if (left == right)
    {
        return entries[left].value;
    }

    float k1 = entries[left].key;
    float v1 = entries[left].value;
    float k2 = entries[right].key;
    float v2 = entries[right].value;
    float ratio = (key - k1) / (k2 - k1);
    return v1 + ratio * (v2 - v1);
}
