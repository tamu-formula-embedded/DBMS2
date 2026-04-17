/** 
 * 
 * Distributed BMS      Host<->Network Byteorder Utilities
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#include "eswap.h"

void memcpy_eswap2(void* dst, void* src, size_t n)
{
    size_t i;
    for (i = 0; i < (n - n % 2); i += 2)
    {
        ((uint8_t*)dst)[i] = ((uint8_t*)src)[i + 1];
        ((uint8_t*)dst)[i + 1] = ((uint8_t*)src)[i];
    }
    if (i != n) ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
}

void memcpy_eswap4(void* dst, void* src, size_t n)
{
    size_t i;
    for (i = 0; i < (n - n % 2); i += 2)
    {
        ((uint8_t*)dst)[i + 0] = ((uint8_t*)src)[i + 3];
        ((uint8_t*)dst)[i + 1] = ((uint8_t*)src)[i + 2];
        ((uint8_t*)dst)[i + 2] = ((uint8_t*)src)[i + 1];
        ((uint8_t*)dst)[i + 3] = ((uint8_t*)src)[i + 0];
    }
    for (; i < n; i++)
    {
        ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
    }
}

uint16_t be16_to_u16(const uint8_t b[2])
{
    return ((uint16_t)b[0] << 8) | ((uint16_t)b[1]);
}
int16_t be16_to_i16(const uint8_t b[2])
{
    return (int16_t)be16_to_u16(b);
}

uint32_t be32_to_u32(const uint8_t b[4])
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | ((uint32_t)b[3]);
}
int32_t be32_to_i32(const uint8_t b[4])
{
    return (int32_t)be32_to_u32(b);
}

uint64_t be48_to_u64(const uint8_t b[6])
{
    return ((uint64_t)b[0] << 40) | ((uint64_t)b[1] << 32) | ((uint64_t)b[2] << 24) | ((uint64_t)b[3] << 16) |
           ((uint64_t)b[4] << 8) | ((uint64_t)b[5]);
}

int64_t be48_to_i64(const uint8_t b[6])
{
    uint64_t v = be48_to_u64(b);
    /* Sign bit is bit 47; sign-extend to 64 bits if set */
    if (v & (1ULL << 47))
    {
        v |= ~((1ULL << 48) - 1);
    }
    return (int64_t)v;
}

uint64_t be64_to_u64(const uint8_t b[8])
{
    return ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32) |
           ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) | ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);
}

int64_t be64_to_i64(const uint8_t b[8])
{
    return (int64_t)be64_to_u64(b);
}
