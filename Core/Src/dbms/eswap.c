#include "eswap.h"

void memcpy_eswap2(void* dst, void* src, size_t n)
{
    size_t i;
    for (i = 0; i < (n - n % 2); i += 2)
    {
        ((unsigned char*)dst)[i] = ((unsigned char*)src)[i+1];
        ((unsigned char*)dst)[i+1] = ((unsigned char*)src)[i];
    }
    if (i != n)
        ((unsigned char*)dst)[i] = ((unsigned char*)src)[i];
}

void memcpy_eswap4(void* dst, void* src, size_t n)
{
    size_t i;
    for (i = 0; i < (n - n % 2); i += 2)
    {
        ((unsigned char*)dst)[i+0] = ((unsigned char*)src)[i+3];
        ((unsigned char*)dst)[i+1] = ((unsigned char*)src)[i+2];
        ((unsigned char*)dst)[i+2] = ((unsigned char*)src)[i+1];
        ((unsigned char*)dst)[i+3] = ((unsigned char*)src)[i+0];
    }
    for (; i < n; i++)
    {
        ((unsigned char*)dst)[i] = ((unsigned char*)src)[i];
    }
}
