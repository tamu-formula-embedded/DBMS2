//  
//  Copyright (c) Texas A&M University.
//  
#include "led_controller.h"

typedef struct _LedMapping {
    GPIO_TypeDef* rx;
    uint16_t rp;
    GPIO_TypeDef* gx;
    uint16_t gp; 
} LedMapping;

void LedSet(Led led, LedColor color) {

    static LedMapping leds[] = {
        { GPIOC, GPIO_PIN_6, GPIOC, GPIO_PIN_7 },
        { GPIOC, GPIO_PIN_8, GPIOC, GPIO_PIN_9 },
        { GPIOA, GPIO_PIN_8, GPIOA, GPIO_PIN_9 }
    };

    HAL_GPIO_WritePin(leds[led].rx, leds[led].rp, color & 0b01);
    HAL_GPIO_WritePin(leds[led].gx, leds[led].gp, color & 0b10);
}