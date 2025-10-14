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
#include "queue.h"

#define CLAMP_U16(x) ((uint16_t)((x) < 0 ? 0 : ((x) > 65535 ? 65535 : (x))))

#define MIN(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a < _b ? _a : _b; })

#define MAX(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a > _b ? _a : _b; })

#define CLAMP(x, low, high) \
    ({ __typeof__(x) _x = (x); \
       __typeof__(low) _low = (low); \
       __typeof__(high) _high = (high); \
       _x < _low ? _low : (_x > _high ? _high : _x); })

#ifdef __vscode__
typedef int int32_t;
typedef unsigned int uint32_t;
#endif

#include "lut.h"
#include "crc.h"
#include "eswap.h"
#include "ma.h"

#ifdef SIM
#include "sim.h"
#else
#include "stm32f4xx_hal.h"
#endif

#endif
