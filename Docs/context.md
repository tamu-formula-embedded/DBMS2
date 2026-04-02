# Context

**Source:** `Core/Src/dbms/context.h`

## Overview

`context.h` is the central header that defines the entire global state of the DBMS firmware. There is one `DbmsCtx` instance, allocated at startup by `DbmsAlloc()`, that is threaded through every module. Rather than using global variables scattered across files, all shared state lives in this single struct — making dependencies explicit and the system state inspectable in one place.

This file also contains the key compile-time topology constants (`N_SEGMENTS`, `N_GROUPS_PER_SIDE`, etc.) and the definitions for all major enumerations and sub-structs.

---

## Top-Level Topology Constants

```c
#define ITER_TARGET_HZ      20     // Main loop target: 20 Hz = 50 ms/iter
#define SPLIT_STACK_OPS     1      // Interleave voltage/temp ops every other iter

#define N_SEGMENTS          2      // COM bus chains
#define N_SIDES_PER_SEG     2      // Monitor chips per chain
#define N_MONITORS_PER_SIDE 1      // (reserved for future multi-chip sides)
#define N_GROUPS_PER_SIDE   13     // Voltage groups measurable per chip
#define N_TEMPS_PER_MONITOR 13     // Temperature sensors per chip (after MUX)
#define N_P_GROUP           3      // Cells in parallel per group
```

Derived constants (computed automatically):
```c
#define N_SIDES      (N_SEGMENTS * N_SIDES_PER_SEG)    // = 4
#define N_MONITORS   (N_SIDES * N_MONITORS_PER_SIDE)   // = 4 total monitor chips
#define N_STACKDEVS  (N_MONITORS + 1)                  // = 5 (4 monitors + 1 bridge)
```

---

## System State Enum

```c
typedef enum _DbmsState {
    DBMS_ACTIVE   = 0,   // Normal discharge operation
    DBMS_SHUTDOWN,       // Shutting down (stack sleeping)
    DBMS_CHARGING        // Charging mode active
} DbmsState;
```

The `DbmsState` is managed in `dbms.c` and gates which operations run each iteration.

---

## Charging State Enum

```c
typedef enum _ChargingState {
    CH_NO_CONN = 0,       // No J1772 connection detected
    CH_CHARGING,          // Actively charging (Elcon enabled)
    CH_WAIT_1,            // Waiting after charge cycle before balancing
    CH_BALANCING_ODDS,    // Balancing odd-indexed cells
    CH_BALANCING_EVENS,   // Balancing even-indexed cells
    CH_WAIT_2,            // Waiting after balancing before resuming charge
    CH_COMPLETE           // Charging complete (all cells at target)
} ChargingState;
```

See [Charging](charging.md) for state machine details.

---

## Key Sub-Structs

### `HwCtx` — Hardware Peripherals
Holds HAL handles for all STM32 peripherals:
- `adc` — ADC for PP voltage reading
- `timer`, `timer_pwm_1` — loop timing and PWM (fan, CP read)
- `uart` — bridge chip UART
- `i2c` — EEPROM
- `can`, `can_tx_header`, `can_tx_mailbox` — CAN bus

### `CellMonitorState` — Per-Side Cell Data
One instance per `N_SIDES` (4 total):
```c
typedef struct _CellMonitorState {
    float voltages[N_GROUPS_PER_SIDE];      // 13 voltages (V)
    float temps[N_TEMPS_PER_SIDE];          // 13 temperatures (°C)
    bool  cells_to_balance[N_GROUPS_PER_SIDE]; // Which groups to balance
} CellMonitorState;
```

### `Stats` — Runtime Counters
Aggregates CAN frame counts, loop timing, stack frame counts, bad CRC counts, min/max/avg voltage and temperature, pack voltage, and other diagnostics. Broadcast periodically over CAN via [VInterface](vinterface.md).

### `Model` — ECM Battery Model Output
Output of the [Model](model.md) module computed each iteration:
```c
typedef struct _Model {
    float Q;       // Cumulative charge (As)
    float z_oc;    // SoC estimate — open circuit path (0.0–1.0)
    float z_rc;    // SoC estimate — RC model path (0.0–1.0)
    float V_oc;    // Open circuit voltage (V)
    float R_oc;    // Internal resistance from OC path (DCIR)
    float R0, R1, R2;  // ECM series resistances (Ω)
    float R_rc;    // Total ECM resistance = R0+R1+R2 (Ω)
    float R_cell;  // Max(R_oc, R_rc) — worst-case resistance used for limiting
    float I_lim;   // Computed current limit (A)
} Model;
```

### `Snapshot` — Blackbox Event Record
A point-in-time capture of system state written to the circular blackbox buffer each iteration. See [Blackbox](blackbox.md).

### `CurrentSensor` — IVT-S Data
Holds current (mA), voltage (mV), power (W), charge (As), energy (Wh) from the IVT-S sensor. Also contains a 100-sample moving average filter for current (`ima`).

### `Timing` — Loop Timestamps
Tracks timestamps for heartbeat watchdog, LED blink, telemetry batching, wakeup time, overtemperature timer, and current pulse timer.

### `Flags` — Single-Bit State Flags
Boolean flags used for inter-iteration communication:

| Flag | Set By | Cleared By | Meaning |
|------|--------|-----------|---------|
| `active` | DBMS init / heartbeat | Shutdown logic | System is in active operation |
| `need_to_reset_qstats` | ? | DBMS iter | Charge stats need reset |
| `telem_enable` | Config CAN | Config CAN | Telemetry enabled |
| `need_to_save_faults` | Faults | DBMS iter | Fault state dirty, needs EEPROM write |
| `need_to_save_blackbox` | Faults (hard fault) | DBMS iter | Blackbox needs EEPROM write |
| `req_fault_clear` | CAN RX | DBMS iter | Remote requested fault clear |
| `need_to_sync_settings` | CFG_SET | DBMS iter | Settings changed, needs EEPROM write |
| `m_led_on` | LED blink timer | LED blink timer | Current LED blink state |
| `has_balanced` | Charging | Charging | At least one balancing cycle has run |
| `shutdown_requested` | Heartbeat watchdog | DBMS reset | Graceful shutdown requested |
| `shutdown_stack_requested` | DBMS iter | DBMS iter | Stack sleep command sent |

### `FaultState` — Fault Tracking
```c
typedef struct _FaultState {
    uint32_t controller_mask;              // Active controller faults (11 bits)
    uint8_t  bridge_fault_summary;         // BQ79600 fault summary
    uint32_t bridge_faults;                // Full bridge fault register
    uint8_t  monitor_fault_summary[N_MONITORS]; // Per-monitor fault bytes
    bool     had_fault;                    // Latched: any fault has occurred
    uint8_t  monitor_bad_crcs[N_MONITORS]; // CRC error counts per monitor
    uint8_t  monitor_total_frames[N_MONITORS]; // Total frames per monitor
} FaultState;
```

### `ChargeStats` — Charge Accumulation
Tracks cumulative charge dispensed from/to the pack, used for SoC initialization and drift compensation. Persisted to EEPROM every 20 seconds during active operation.

### `Charging` — Charging State Machine Data
All runtime state for the charging state machine including timestamps, accumulated pre-balance voltage samples, and minimum/maximum cell voltages seen during the wait period. See [Charging](charging.md).

### `Profiling` — Loop Timing Checkpoints
Ten timestamp checkpoints (`T0`–`T10`) recorded at key points in the main loop iteration, enabling detailed performance profiling of where loop time is spent.

---

## `DbmsCtx` — The Full Context

The top-level struct that holds everything:

```c
typedef struct _DbmsCtx {
    HwCtx           hw;
    LutData         data;           // Thermistor LUT
    LedState        led_state;
    DbmsSettings*   settings;       // Pointer to settings in EEPROM shadow
    CellMonitorState cell_states[N_SIDES];  // 4 × {13 voltages, 13 temps}
    uint8_t         mux_selector;   // Current MUX channel (0–3)
    Realtime        realtime;       // Global/local timestamp for real-time sync
    Timing          timing;
    Flags           flags;
    Stats           stats;
    CurrentSensor   current_sensor;
    uint32_t        max_current_ma;
    ChargeStats     qstats;
    FaultState      faults;
    uint16_t        faults_crc;
    Model           model;
    uint16_t        can_log_ordering_index;
    uint32_t        last_can_err;
    bool            precharged;
    Blackbox        blackbox;
    Elcon           elcon;
    J1772           j1772;
    Charging        charging;
    Profiling       profiling;
} DbmsCtx;
```

---

## Interactions

`context.h` is included by virtually every module. The `DbmsCtx*` pointer is passed as the first argument to most functions in the system. It is the single source of truth for all shared state.
