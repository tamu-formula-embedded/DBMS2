//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _LEDCTL_H_
#define _LEDCTL_H_

#include "common.h"
#include "context.h"


typedef enum _LedColor {
    LED_OFF,
    LED_RED,
    LED_GREEN,
    LED_YELLOW
} LedColor;

typedef enum _Led {
    LED6 = 0,
    LED7,
    LED8
} Led;

void LedSet(Led led, LedColor color);

#endif