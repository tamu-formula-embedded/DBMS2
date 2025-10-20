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

#define CLAMP_U16(x)    (CLAMP(x, 0, 65535))
//#define CLAMP_U16(x) ((uint16_t)((x) < 0 ? 0 : ((x) > 65535 ? 65535 : (x))))

#ifdef __vscode__
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

#endif