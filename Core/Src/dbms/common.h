//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>

#ifdef __vscode__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif

#ifdef SIM 
    #include "sim.h"
#else
    #include "stm32f4xx_hal.h"
#endif

#endif

