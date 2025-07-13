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
    I2C_HandleTypeDef* i2c;

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
    #define CFGID_MAX_ALLOWED_PACK_VOLTS    0x01
    uint16_t max_allowed_pack_voltage;

    // How many miliseconds since the last heartbeat do we wait
    // before initiating a shutdown
    #define CFGID_QUIET_MS_BEFORE_SHUTDOWN  0x02
    uint32_t quiet_ms_before_shutdown;


} DbmsSettings;

typedef struct _DbmsCtx {

    HwCtx hw;                   // Holds points to hardware peripherals

    DbmsState       req_state;        // the state we want
    DbmsState       cur_state;        // the state we are in
    DbmsSettings    settings;

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
    bool        led_show_error;

} DbmsCtx;


#endif
