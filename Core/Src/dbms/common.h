//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _COMMON_H_
#define _COMMON_H_

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <math.h>

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))


#ifdef __vscode__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif


#include "../lib/lib.h"

#ifdef SIM 
    #include "sim.h"
#else
    #include "stm32f4xx_hal.h"
#endif

#endif

