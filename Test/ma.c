#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define MIN(a, b) (a < b ? a : b)

typedef struct {
    size_t i, cap, size;
    int64_t sum;
    void* vec;
} ma_t;

void ma_init(ma_t* ma, void* vec, size_t cap)
{
    ma->i = ma->size = ma->sum = 0;
    ma->cap = cap;
    ma->vec = vec;
}

int32_t ma_calc32(ma_t* ma, int32_t n)
{
    int32_t* vec = (int32_t*)ma->vec;
    ma->size = MIN(ma->size + 1, ma->cap);
    ma->sum -= vec[ma->i];
    ma->sum += n;
    vec[ma->i] = n;
    ma->i = (++ma->i) % ma->cap;
    return (ma->sum / ma->size);
}

int main()  
{
    // Initialize a moving-average with length 100 and int32_t type
    ma_t ma;
    int32_t ma_buf[100] = {0};
    ma_init(&ma, ma_buf, 100);

    for (int i = 0; i < 1000; i++)
    {
        int32_t o = ma_calc32(&ma, 10 * i);
        int32_t e = i < 100 ? 5 * i : 10 * i - 495;
        printf("%4d\t%4d\t%4d\n", i, o, e);
        if (e != o)
        {
            printf("A testcase has failed!\n");
            return 1;
        }
    }
    printf("All testcases passed!\n"); 
    return 0;
}




