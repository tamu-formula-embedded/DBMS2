/** 
 * 
 * Distributed BMS      Host<->Network Byteorder Utils
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */
#ifndef _ESWAP_H_
#define _ESWAP_H_

#include <stdint.h>
#include <stdlib.h>

#define REVERSE_ARRAY_T(arr, len, type)             \
    do {                                            \
        size_t i, j;                                \
        for (i = 0, j = (len) - 1; i < j; ++i, --j) \
        {                                           \
            type temp = arr[i];                     \
            arr[i] = arr[j];                        \
            arr[j] = temp;                          \
        }                                           \
    } while (0)

void memcpy_eswap2(void* dst, void* src, size_t n);
void memcpy_eswap4(void* dst, void* src, size_t n);

/* 16-bit */
uint16_t be16_to_u16(const uint8_t b[2]);
int16_t  be16_to_i16(const uint8_t b[2]);

/* 32-bit */
uint32_t be32_to_u32(const uint8_t b[4]);
int32_t  be32_to_i32(const uint8_t b[4]);

/* 48-bit (stored in 64-bit container) */
uint64_t be48_to_u64(const uint8_t b[6]);
int64_t  be48_to_i64(const uint8_t b[6]);

/* 64-bit */
uint64_t be64_to_u64(const uint8_t b[8]);
int64_t  be64_to_i64(const uint8_t b[8]);

#endif