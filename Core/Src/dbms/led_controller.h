//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _LEDCTL_H_
#define _LEDCTL_H_

#include "common.h"
#include "context.h"

/**
*  Enum for individual LED states
*/
typedef enum _LedState {
    LED_STATE_OFF = 0,
    LED_STATE_SOLID_RED,
    LED_STATE_SOLID_GREEN,
    LED_STATE_SOLID_YELLOW,
    LED_STATE_FAST_BLINK_RED,
    LED_STATE_FAST_BLINK_GREEN,
    LED_STATE_FAST_BLINK_YELLOW,
    LED_STATE_SLOW_BLINK_RED,
    LED_STATE_SLOW_BLINK_GREEN,
    LED_STATE_SLOW_BLINK_YELLOW,
    LED_STATE_COUNT // for bounds checking
} LedState;

/**
*  Enum for actual LED colors
*/
typedef enum _LedColor {
    LED_OFF = 0,
    LED_RED,
    LED_GREEN,
    LED_YELLOW,
} LedColor;

/**
*  Enum for system LED states (all 3 LEDs)
*  these combos are mapped in implementation file's system_led_patterns
*/
typedef enum _DbmsLedState {
    ERROR = 0,
    ACTIVE,
    IDLE,
    COMM_ERROR,
    INIT,
    // ...
    DBMS_LED_STATE_COUNT
} DbmsLedState;

/**
*  Enum for individual LEDs, used to index
*/
typedef enum _Led {
    LED6 = 0,
    LED7,
    LED8
} Led;

void SetLedState(DbmsLedState state);

void LedSet(Led led, LedColor color);

#endif