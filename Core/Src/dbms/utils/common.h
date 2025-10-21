/** 
 * 
 * Distributed BMS      Common utilities 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Minimuim of two elements a, b
 */
#define MIN(A, B)   ((A) < (B) ? (A) : (B))

/**
 * Maximum of two elements a, b
 */       
#define MAX(A, B)   ((A) > (B) ? (A) : (B))

/**
 * Clamp x between low and high
 */
#define CLAMP(X, LOW, HIGH) X < LOW ? LOW : (X > HIGH ? HIGH : X);



#ifdef __vscode__

// typedef int int32_t;
// typedef unsigned int uint32_t;
#define int32_t     int
#define uint32_t    unsigned int

#endif

#include "stm32f4xx_hal.h"

#include "crc.h"
#include "eswap.h"
#include "lut.h"
#include "ma.h"

#endif