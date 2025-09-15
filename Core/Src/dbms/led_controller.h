//
//  Copyright (c) Texas A&M University.
//
#ifndef _LEDCTL_H_
#define _LEDCTL_H_

#include "common.h"
#include "context.h"

/**
 *  Enum for system LED states (all 3 LEDs)
 *  these combos are mapped in implementation file's system_led_patterns
 *
 *  (fwd declared)
 */
enum _LedState
{
    LED_ERROR = 0,
    LED_ACTIVE,
    LED_IDLE,
    LED_COMM_ERROR,
    LED_INIT,
    LED_CHARGING,
    LED_BALANCING,
    // ...
    LED_STATE_COUNT
};

/**
 *  Enum for actual LED colors
 */
typedef enum _LedColor
{
    LED_OFF = 0,
    LED_RED,
    LED_GREEN,
    LED_YELLOW,
} LedColor;

/**
 *  Enum for individual LEDs, used to index
 */
typedef enum _Led
{
    LED6 = 0,
    LED7,
    LED8
} Led;

void ProcessLedAction(DbmsCtx* ctx);

void LedSet(Led led, LedColor color);

#endif
