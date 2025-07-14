//  
//  Copyright (c) Texas A&M University.
//  
#include "led_controller.h"


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
    [ERROR]  = { LED_STATE_SOLID_RED, LED_STATE_SOLID_RED, LED_STATE_SOLID_RED },
    [ACTIVE] = { LED_STATE_SOLID_GREEN, LED_STATE_SOLID_GREEN, LED_STATE_SOLID_GREEN },
    [IDLE]   = { LED_STATE_SOLID_YELLOW, LED_STATE_SOLID_YELLOW, LED_STATE_SOLID_YELLOW },
};

/**
*  Helper function to set the individual LED color using HAL
*/
void LedSet(Led led, LedColor color) {
    HAL_GPIO_WritePin(leds[led].rx, leds[led].rp, color & 0b01);
    HAL_GPIO_WritePin(leds[led].gx, leds[led].gp, color & 0b10);
}

/**
*  Sets the state of all LEDs based on the passed in system state
*/
void SetLedState(DbmsLedState state){
    for (size_t i = 0; i < 3; i++)
    {
        // Loop through each LED in the patterns table
        // and set it to the appropriate state
        LedSet((Led)i, system_led_patterns[state][i]);
    }
}


