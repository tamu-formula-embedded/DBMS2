
//
//  Copyright (C) Texas A&M University
//
//  ctx -- Context
//
#ifndef _H_CTX_
#define _H_CTX_

#include "common.h"

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