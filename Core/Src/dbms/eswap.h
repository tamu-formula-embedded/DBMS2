//
//  Copyright (c) Texas A&M University.
//
#ifndef _H_ESWAP_H
#define _H_ESWAP_H

#include <stdlib.h>

void memcpy_eswap2(void* dst, void* src, size_t n);
void memcpy_eswap4(void* dst, void* src, size_t n);

#endif