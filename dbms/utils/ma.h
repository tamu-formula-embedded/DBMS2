/** 
 * 
 * Distributed BMS      Moving Average Calculation 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */
#ifndef _MA_H_
#define _MA_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/*
    Moving Average

    Computes a moving-average of an incoming variable over some 
    given capacity. If n samples < capacity, returns the cumulative
    average over the current number of samples.

    Initialize a moving-average with length 100 and int32_t type

        ma_t ma;
        int32_t ma_buf[100] = {0};
        ma_init(&ma, ma_buf, 100);

    Compute an int32_t moving average (uses integer division)
    This is best for values that can be scaled up to have large
    fixed precision (ex. total current in milli-amps)

        int32_t moving_avg = ma_calc_i32(new_val);
*/

typedef struct {
    size_t i, cap, size;
    int64_t sum;
    void* vec;
} ma_t;

void ma_init(ma_t* ma, void* vec, size_t cap);

// todo:    add more calc functions
//          could use macros to unroll or something?
int32_t ma_calc_i32(ma_t* ma, int32_t n);

#endif