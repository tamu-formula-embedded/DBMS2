//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _H_CTX_
#define _H_CTX_

#include "common.h"


// USER DEFINED UNIQUE TO EACH BATTERY
#define N_SEGMENTS 1
#define N_MONITORS_PER_SEG 4
#define N_GROUPS 14
#define N_TEMPS 14
// DONT CHANGE AFTER THIS

#define N_STACKDEVS (N_SEGMENTS * N_MONITORS_PER_SEG + 1)
// #define STACKITER(I) for (int I = 0; I < N_STACKDEVS; I++)

typedef enum _DbmsState {
    DBMS_ACTIVE = 0,
    DBMS_SHUTDOWN,
} DbmsState;

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


typedef struct _CellMonitorState {
    uint16_t voltages[N_GROUPS];
    uint16_t temps[N_TEMPS];
} CellMonitorState;


typedef struct _DbmsSettings {

    // The maximum voltage of the pack or else a fault is thrown
    uint16_t max_allowed_pack_voltage;

} DbmsSettings;

typedef struct _DbmsCtx {
    DbmsState state;
    DbmsSettings settings;

    // 2D grid representing the battery
    // example with 5 segments and 4 monitors each
    // [
    //   seg1 [ monitor1, monitor2, monitor3, monitor4 ]
    //   seg2 [ monitor1, monitor2, monitor3, monitor4 ]
    //   seg3 [ monitor1, monitor2, monitor3, monitor4 ]
    //   seg4 [ monitor1, monitor2, monitor3, monitor4 ]
    //   seg5 [ monitor1, monitor2, monitor3, monitor4 ]
    // ]
    CellMonitorState cell_states[N_SEGMENTS][N_MONITORS_PER_SEG];

    uint64_t iterct;

} DbmsCtx;


#endif
