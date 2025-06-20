//
//  Copyright (C) Texas A&M University
//
//  Hardware Abstraction Layer Interface
//  Switches between using the provided STM32 HAL
//  and the simulated HAL
//
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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

