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

// fwd definition -- perf_counters.h
typedef struct _PerfCounters PerfCounters;  

// Hardware context stores prts 
// to peripheral interfaces
typedef struct _HwCtx {
    ADC_HandleTypeDef* adc;
    TIM_HandleTypeDef* timer;
    UART_HandleTypeDef* uart;
    I2C_HandleTypeDef* i2c;

    CAN_HandleTypeDef* can;
    CAN_TxHeaderTypeDef can_tx_header;
    uint32_t can_tx_mailbox;
} HwCtx;


typedef struct _CellMonitorState {
    uint16_t voltages[N_GROUPS];
    uint16_t temps[N_TEMPS];
} CellMonitorState;

// fwd definition -- settings.h
typedef enum _UserSettingIndex UserSettingIndex; 
typedef struct _DbmsSettings DbmsSettings;

// fwd definition for enum -- led_controller.h
typedef int32_t LedState;   

typedef struct _DbmsCtx {

    HwCtx hw;                   // Holds points to hardware peripherals

    DbmsState       req_state;        // the state we want
    DbmsState       cur_state;        // the state we are in
    LedState        led_state;        // the state of the LEDs

    DbmsSettings*   settings;         // struct fwd defs has to be a ptr
    PerfCounters*   perfctrs;  

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

    uint64_t    iterct;
    uint64_t    last_rx_heartbeat;

    bool        need_to_sync_settings;

} DbmsCtx;


#endif
