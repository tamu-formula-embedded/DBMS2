#include "ma.h"

void ma_init(ma_t* ma, void* vec, size_t cap)
{
    ma->i = ma->size = ma->sum = 0;
    ma->cap = cap;
    ma->vec = vec;
}

#ifndef MIN
#define MIN(a, b)   (((a) < (b)) ? (a) : (b))
#endif

int32_t ma_calc_i32(ma_t* ma, int32_t n)
{
    int32_t* vec = (int32_t*)ma->vec;
    ma->size = MIN(ma->size + 1, ma->cap);
    ma->sum -= vec[ma->i];
    ma->sum += n;
    vec[ma->i] = n;
    ma->i = (ma->i + 1) % ma->cap;
    return (ma->sum / ma->size);
}

