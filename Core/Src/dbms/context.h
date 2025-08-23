//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _H_CTX_
#define _H_CTX_

#include "common.h"

#define ITER_TARGET_HZ 25

// USER DEFINED UNIQUE TO EACH BATTERY
#define N_SEGMENTS 2
#define N_MONITORS_PER_SEG 4
#define N_GROUPS 14
#define N_TEMPS 7
// DONT CHANGE AFTER THIS

#define N_MONITORS (N_SEGMENTS * N_MONITORS_PER_SEG)
#define N_STACKDEVS (N_MONITORS + 1)    // technically "bus devs"

#define ADDR_BCAST_TO_STACK(BCAST_ADDR) (BCAST_ADDR - 1)
#define ADDR_STACK_TO_BCAST(STACK_ADDR) (STACK_ADDR + 1)


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
    float voltages[N_GROUPS];
    float temps[N_TEMPS];
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

    CellMonitorState cell_states[N_MONITORS];

    uint64_t    last_rx_heartbeat;
    uint64_t    iter_start_us;
    uint64_t    iter_end_us;
    uint64_t    M_LED_BLINK_TS;

    struct {
        uint64_t iters;

// #define N_HISTORIC_LOOPTIMES 16
//         wrap_queue_t looptimes_q;
//         uint32_t looptimes_d[N_HISTORIC_LOOPTIMES];

        uint32_t n_tx_can_frames;
        uint32_t n_rx_can_frames;
        uint32_t n_unmatched_can_frames;
        uint32_t n_tx_can_drop_timeout;
        uint32_t n_tx_can_fail;

        uint32_t looptime;
        uint32_t end_delay;
        // uint32_t n_overruns;
    } stats;

     struct {
        uint64_t current_ma;
        uint64_t voltage1_mv;
        uint64_t voltage2_mv;
        uint64_t voltage3_mv;
        uint64_t power_w;
        uint64_t charge_as;
        uint64_t energy_wh;
    } isense; // current = I, sense = sensor?

    uint8_t     last_can_err;
    bool        need_to_sync_settings;
    bool        M_LED_ON;

} DbmsCtx;


#endif
