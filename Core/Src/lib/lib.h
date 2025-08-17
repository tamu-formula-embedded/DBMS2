//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _LIB_H_
#define _LIB_H_

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <math.h>

#ifdef __vscode__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif

#include "crc.h"
#include "exp_arr.h"

typedef struct exp_arr ExpArr;

// performs memcpy and reverses endianness 
// of every 2 byte int
void memcpy_eswap2(void* dst, void* src, size_t n);

// performs memcpy and reverses endianness 
// of every 4 byte int 
void memcpy_eswap4(void* dst, void* src, size_t n);

#endif