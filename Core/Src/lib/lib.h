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

#endif