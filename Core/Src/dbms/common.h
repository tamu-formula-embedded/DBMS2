//
//  Copyright (c) Texas A&M University.
//
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

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#ifdef __vscode__
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

#include "lut.h"
#include "crc.h"
#include "eswap.h"

#ifdef SIM
#include "sim.h"
#else
#include "stm32f4xx_hal.h"
#endif

#endif
