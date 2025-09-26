//
//  Copyright (c) Texas A&M University.
//
#ifndef _H_ESWAP_H
#define _H_ESWAP_H

#include <stdlib.h>
#include <stdint.h>

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