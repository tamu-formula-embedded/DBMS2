//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _H_CTX_
#define _H_CTX_

#include "common.h"


// USER DEFINED UNIQUE TO EACH BATTERY
#define N_SEGMENTS 1
#define N_MONITORS_PER_SEG 2
#define N_GROUPS 10
#define N_TEMPS 8
// DONT CHANGE AFTER THIS

#define N_STACKDEVS (N_SEGMENTS * N_MONITORS_PER_SEG + 1)
// #define STACKITER(I) for (int I = 0; I < N_STACKDEVS; I++)


// Hardware context stores prts 
// to peripheral interfaces
typedef struct _HwCtx {
    ADC_HandleTypeDef* adc;
    TIM_HandleTypeDef* timer;
    UART_HandleTypeDef* uart;

    CAN_HandleTypeDef* can;
    CAN_TxHeaderTypeDef can_tx_header;
    uint32_t can_tx_mailbox;
} HwCtx;

typedef struct _DbmsSettings {

    // The maximum voltage of the pack or else a fault is thrown
    uint16_t max_allowed_pack_voltage;

} DbmsSettings;

typedef struct _DbmsCtx {

    DbmsSettings settings;

} DbmsCtx;


#endif