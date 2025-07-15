//  
//  Copyright (c) Texas A&M University.
//  
#include "led_controller.h"

#define FAST_BLINK_INTERVAL 100
#define SLOW_BLINK_INTERVAL 500

/**
*  LedMapping struct maps a logical LED to its physical hardware pins. 
*  It contains the GPIO port and pin number for both the red and green components of a single LED
*/
typedef struct _LedMapping {
    GPIO_TypeDef* rx;
    uint16_t rp;
    GPIO_TypeDef* gx;
    uint16_t gp; 
} LedMapping;

/**
*  Array of LED mappings for each LED
*  LED 0: Red on PC6, Green on PC7
*  LED 1: Red on PC8, Green on PC9
*  LED 2: Red on PA8, Green on PA9
*/
static const LedMapping leds[] = {
    { GPIOC, GPIO_PIN_6, GPIOC, GPIO_PIN_7 },
    { GPIOC, GPIO_PIN_8, GPIOC, GPIO_PIN_9 },
    { GPIOA, GPIO_PIN_8, GPIOA, GPIO_PIN_9 }
};

/**
*  Table mapping each system state to per-LED states
*/
static const LedState system_led_patterns[DBMS_LED_STATE_COUNT][3] = {
    [ERROR]      = { LED_STATE_SOLID_RED, LED_STATE_SOLID_RED, LED_STATE_SOLID_RED },
    [ACTIVE]     = { LED_STATE_SOLID_GREEN, LED_STATE_SOLID_GREEN, LED_STATE_SOLID_GREEN },
    [IDLE]       = { LED_STATE_SOLID_YELLOW, LED_STATE_SOLID_YELLOW, LED_STATE_SOLID_YELLOW },
    [COMM_ERROR] = { LED_STATE_SOLID_RED, LED_STATE_SOLID_RED, LED_STATE_FAST_BLINK_RED },
    [INIT]       = { LED_STATE_SOLID_GREEN, LED_STATE_SOLID_GREEN, LED_STATE_SLOW_BLINK_GREEN },
};

/**
*  Table mapping each LED state their colors
*/
static const LedColor ledstate_to_color[] = {
    [LED_STATE_OFF]              = LED_OFF,
    [LED_STATE_SOLID_RED]        = LED_RED,
    [LED_STATE_SOLID_GREEN]      = LED_GREEN,
    [LED_STATE_SOLID_YELLOW]     = LED_YELLOW,
    [LED_STATE_FAST_BLINK_RED]   = LED_RED,
    [LED_STATE_FAST_BLINK_GREEN] = LED_GREEN,
    [LED_STATE_FAST_BLINK_YELLOW]= LED_YELLOW,
    [LED_STATE_SLOW_BLINK_RED]   = LED_RED,
    [LED_STATE_SLOW_BLINK_GREEN] = LED_GREEN,
    [LED_STATE_SLOW_BLINK_YELLOW]= LED_YELLOW
};

/**
*  Function the dbms iter loop will actually call
*  Handles blinking and non-blinking states
*/
void ProcessLedAction(DbmsCtx* ctx) {
    static uint32_t last_blink_time = 0;
    static uint8_t blink_on = 0;
    uint32_t now = HAL_GetTick();

    int blink_needed = 0;
    uint32_t blink_interval = 0;

    // determine if any LED needs blinking, and set the interval
    for(int i = 0; i < 3; ++i){
        LedState state = system_led_patterns[ctx->led_state][i];
        switch(state){
            case LED_STATE_FAST_BLINK_RED:
            case LED_STATE_FAST_BLINK_GREEN:
            case LED_STATE_FAST_BLINK_YELLOW:
                blink_needed = 1;
                blink_interval = FAST_BLINK_INTERVAL;
                break;
            case LED_STATE_SLOW_BLINK_RED:
            case LED_STATE_SLOW_BLINK_GREEN:
            case LED_STATE_SLOW_BLINK_YELLOW:
                blink_needed = 1;
                blink_interval = SLOW_BLINK_INTERVAL;
                break;
            default:
                break;
        }
    }

    // handle blink timing
    if(blink_needed && (now - last_blink_time >= blink_interval)){
        blink_on = !blink_on;
        last_blink_time = now;
    }

    // set each LED according to its state and blink logic
    for(int i = 0; i < 3; ++i){
        LedState state = system_led_patterns[ctx->led_state][i];
        LedColor color = ledstate_to_color[state];

        // if this is a blinking state and we're in the "off" part of the cycle, turn off the LED
        if((state >= LED_STATE_FAST_BLINK_RED && state <= LED_STATE_SLOW_BLINK_YELLOW) && !blink_on){
            color = LED_OFF;
        }
        LedSet((Led)i, color);
    }
}

/**
*  Helper function to set the individual LED color using HAL
*/
void LedSet(Led led, LedColor color) {
    HAL_GPIO_WritePin(leds[led].rx, leds[led].rp, color & 0b01);
    HAL_GPIO_WritePin(leds[led].gx, leds[led].gp, color & 0b10);
}