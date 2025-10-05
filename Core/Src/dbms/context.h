//
//  Copyright (c) Texas A&M University.
//
#ifndef _H_CTX_
#define _H_CTX_

#include "common.h"

#define ITER_TARGET_HZ 10

// USER DEFINED UNIQUE TO EACH BATTERY
#define N_SEGMENTS 5
#define N_SIDES_PER_SEG 2
#define N_MONITORS_PER_SIDE 2
#define N_GROUPS_PER_SIDE 14
#define N_TEMPS_PER_MONITOR 7
#define N_P_GROUP 3
// DONT CHANGE AFTER THIS

#define N_TEMPS_PER_SIDE (N_MONITORS_PER_SIDE * N_TEMPS_PER_MONITOR)
#define N_SIDES (N_SEGMENTS * N_SIDES_PER_SEG)
#define N_MONITORS (N_SEGMENTS * N_SIDES_PER_SEG * N_MONITORS_PER_SIDE)
#define N_STACKDEVS (N_MONITORS + 1) // technically "bus devs"

#define ADDR_BCAST_TO_STACK(BCAST_ADDR) (BCAST_ADDR - 1)
#define ADDR_STACK_TO_BCAST(STACK_ADDR) (STACK_ADDR + 1)

#define N_THERM_V_TO_T_ENTRIES      121
#define N_OCV_ENTRIES               201
#define N_RC_ENTRIES                101
#define N_C_ENTRIES                 101
#define N_I_MA_MEMORIZED            100


// DANGER:  THESE DEBUGS WILL PREVENT THE CONTROLLER FROM WORKING NORMALLY
// #define DEBUG_DO_OVERWRITE_TEMPS_OVER_CAN
// END DANGER ZONE

typedef enum _DbmsState
{
    DBMS_ACTIVE = 0,
    DBMS_SHUTDOWN,
} DbmsState;

// fwd definition -- perf_counters.h
typedef struct _PerfCounters PerfCounters;

typedef enum
{
    NOT_CHARGING = 0,
    CHARGING_ACTIVE,
    CHARGING_PAUSED_FOR_BALANCING,
    CHARGING_COMPLETE,
    CHARGING_ERROR
} ChargingState;

// Hardware context stores prts
// to peripheral interfaces
typedef struct _HwCtx
{
    ADC_HandleTypeDef* adc;
    TIM_HandleTypeDef* timer;
    TIM_HandleTypeDef* timer_pwm_1;
    UART_HandleTypeDef* uart;
    I2C_HandleTypeDef* i2c;

    CAN_HandleTypeDef* can;
    CAN_TxHeaderTypeDef can_tx_header;
    uint32_t can_tx_mailbox;
} HwCtx;

typedef struct _CellMonitorState
{
    float voltages[N_GROUPS_PER_SIDE];
    float temps[N_TEMPS_PER_SIDE];
    bool cells_to_balance[N_GROUPS_PER_SIDE];   // TODO: change to bitmask? prob not
} CellMonitorState;

// fwd definition -- settings.h
typedef enum _UserSettingIndex UserSettingIndex;
typedef struct _DbmsSettings DbmsSettings;

// fwd definition for enum -- led_controller.h
typedef int32_t LedState;

typedef struct _Stats
{
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

    uint32_t n_rx_stack_frames;
    uint32_t n_rx_stack_bad_crcs;
    // uint32_t n_overruns;

    uint32_t n_eeprom_writes;

    float min_v;
    float max_v;
    float avg_v;
    float min_t;
    float max_t;
    float avg_t;
} Stats;

typedef struct _Model   // Outputs from the ECM model
{
    float Q;            // Charge 
    float z_oc;         // % Charge (OC path)
    float z_rc;         // % Charge (RC path)
    float V_oc;         // Open Circuit Voltage
    float R_oc;

    float R0;
    float R1;
    float R2;
    float R_rc;

    float R_cell;
    float I_lim;
} Model;

typedef struct _BlackboxInfo
{
    uint64_t iter;
} BlackboxInfo;

typedef struct _DbmsCtx
{

    HwCtx hw; // Holds points to hardware peripherals

    struct {
        LTE lut_therm_v_to_t[N_THERM_V_TO_T_ENTRIES];
    } data;

    DbmsState req_state; // the state we want
    DbmsState cur_state; // the state we are in
    LedState led_state;  // the state of the LEDs

    DbmsSettings* settings; // struct fwd defs has to be a ptr

    CellMonitorState cell_states[N_SIDES];

    struct {
        uint64_t global_ts;
        uint64_t local_ts;
    } realtime;

    uint64_t last_rx_heartbeat;
    uint64_t iter_start_us;
    uint64_t iter_end_us;
    uint64_t m_led_blink_ts;
    uint64_t batch_telem_ts;
    uint64_t wakeup_ts;

    Stats stats;

    struct
    {
        ChargingState state;
        bool balancing_requested;
    } charging;

    struct
    {
        int32_t current_ma;
        int32_t voltage1_mv;
        int32_t power_w;
        int32_t charge_as;
        int32_t energy_wh;

        struct {
            ma_t ma;
            int32_t buf[N_I_MA_MEMORIZED];
            int32_t current_mavg_ma;
        } ima;      // charge = I, ma = moving average (ang milliamps, so sometimes mavg)

    } isense; // current = I, sense = sensor
    uint32_t max_current_ma;

    uint64_t pl_last_ok_ts;
    uint64_t pl_pulse_t;
    uint64_t overtemp_last_ok_ts;

    struct {
        float       initial;                      // Q0
        float       historic_accumulated_loss;    // QD
        float       accumulated_loss;             // Qd
        uint32_t    initial_set_ts;
    } qstats;                                     // charge stats
    bool need_to_reset_qstats;
 

    struct
    {
        uint32_t controller_mask;
        uint8_t bridge_fault_summary;
        uint32_t bridge_faults;
        uint8_t monitor_fault_summary[N_MONITORS];
        bool had_fault;
    } faults;
    bool need_to_save_faults;
    uint16_t faults_crc;

    Model model;

    uint16_t can_log_ordering_index;
    uint8_t last_can_err;
    bool need_to_sync_settings;
    bool m_led_on;

    // Blackbox pointers for crash analysis
    BlackboxInfo* blackbox_old;
    BlackboxInfo* blackbox_new;

} DbmsCtx;

#endif
