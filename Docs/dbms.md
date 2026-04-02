# DBMS

**Source:** `Core/Src/dbms/dbms.c`, `dbms.h`

## Overview

`dbms.c` is the top-level controller. It owns the system lifecycle (alloc → init → wakeup → iter loop → shutdown), the active/shutdown state machine, and the CAN RX dispatch table. Every module is orchestrated from here.

---

## Lifecycle

```
DbmsAlloc()        // one-time memory setup
DbmsInit()         // peripheral + EEPROM init
    │
    └─► main() calls DbmsIter() at 20 Hz in a tight loop
             │
             └─► DbmsClose() / DbmsErr() on exit or fault
```

### `DbmsAlloc()`
Called before `DbmsInit()`. Assigns the static `mem_settings` buffer to `ctx->settings` and zeroes stats. This ensures the settings pointer is valid before any EEPROM reads.

### `DbmsInit()`
Performs one-time initialization:
- Loads settings from EEPROM (falls back to hardcoded defaults and schedules a write if CRC fails)
- Starts the hardware timer used by `GetUs()`
- Configures CAN filters and starts both CAN peripherals
- Loads the persisted fault state and charge stats (`LoadFaultState`, `LoadQStats`)
- Configures the IVT-S current sensor measurement cycle
- Calls `DataInit()` to build the thermistor voltage→temperature LUT

### `DbmsPerformWakeup()`
Called the first time `DbmsIter()` runs while `flags.active == false`. Performs the full BQ79616 startup sequence:
1. 2-second delay (stack power stabilization)
2. `StackWake()` — sends the UART wake blip and `CTRL1_SEND_WAKE` command twice
3. `StackAutoAddr()` — runs the 6-step auto-addressing procedure
4. `StackSetNumActiveCells()`, `StackSetupGpio()`, `StackSetupVoltReadings()`, `StackConfigTimeout()`, `StackBalancingConfig()`
5. `ConfigCurrentSensor()` — sends IVT-S configuration frames
6. Records `timing.wakeup_ts` — used to defer fault checks by `MS_BEFORE_FAULT_CHECKS` (default 10 s)

### `DbmsPerformShutdown()`
Optionally calls `StackShutdown()` (skipped if `shutdown_stack_requested` is false), saves charge stats to EEPROM, and sets `LED_IDLE`.

---

## Main Loop — `DbmsIter()`

Called at 20 Hz. The sequence within each iteration:

```
1.  [if blackbox.requested]  BlackboxSend() over CAN
2.  [if need_to_sync_settings]  SaveSettings() to EEPROM
3.  [if need_to_reset_qstats]  SaveQStats() to EEPROM
4.  [every 400 iters ≈ 20 s]  PeriodicSaveQStats()
5.  CanTxHeartbeat()  — broadcast settings CRC
6.  Shutdown/wakeup logic:
      if shutdown_requested → DbmsPerformShutdown(), hold fault line
      else if not active   → DbmsPerformWakeup(), set active
7.  DbmsHandleActive()  — volt/temp reads, fault checks
8.  ChargingUpdate()
9.  UpdateModel()
10. BlackboxUpdate()  — snapshot into circular buffer
11. PrechargeUpdate()
12. [if need_to_save_faults]  SaveFaultState()
13. [if need_to_save_blackbox]  BlackboxSaveOnFault()
14. SendPlexMetrics()  — always transmitted
15. [if telem_enable]  SendMetrics(), SendCellVoltages(), SendCellTemps()
16. [HAS_FAN]  UpdateFan()
17. ProcessLedAction() / MonitorLedBlink()
18. SetMuxChannels(iters % 4)  — advance thermistor MUX for next iteration
19. CalcIterDelay()  → HAL_Delay() to pad loop to 50 ms target
```

### Telemetry Enable Gating
`telem_enable` is set true only when a "telembeat" frame (ID `CANID_RX_TELEMBEAT`) has been received within the last 5 seconds. The full metrics + cell data blast is only sent when a PC-side receiver is actively present. `SendPlexMetrics()` (the compact vehicle-CAN subset) is always sent.

### Shutdown vs. Active Transition
The transition is edge-based: `shutdown_requested` is a sticky flag that, once set, keeps the system shut down until explicitly cleared via `CANID_RX_CLEAR_SHUTDOWN`. The system does **not** self-wake on watchdog timeout — shutdown is latched.

---

## `DbmsHandleActive()`

Runs the per-iteration active work:
1. `StackUpdateAllVoltReadings()` + 10 ms delay
2. `StackUpdateAllTempReadings()` + 10 ms delay
3. `FillMissingTempReadings()` if `IGNORE_BAD_THERMS` setting is set
4. `StackCalcStats()` — compute min/avg/max for voltages and temperatures
5. After `MS_BEFORE_FAULT_CHECKS` has elapsed since wakeup:
   - `CtrlClearAllFaults()` if `req_fault_clear` is set
   - `CheckVoltageFaults()`, `CheckCurrentFaults()`, `CheckTemperatureFaults()`
   - `SetFaultLine()` based on `CtrlHasAnyFaults()`
6. `ThrowHardFault()` — updates fault line and sets save/blackbox flags on new faults

The 10 ms `HAL_Delay()` calls between voltage and temperature reads are necessary to allow the BQ79616 ADC to settle.

---

## `DbmsCanRx()` — CAN RX Dispatch

All incoming CAN frames are dispatched here from the RX interrupt. Key behaviors:

| Frame | Action |
|-------|--------|
| `CANID_RX_HEARTBEAT` | Resets watchdog timer, calls `SyncRealTime()` with 64-bit timestamp |
| `CANID_RX_SHUTDOWN_STACK` | Sets `shutdown_requested` + `shutdown_stack_requested` |
| `CANID_RX_CLEAR_SHUTDOWN` | Clears both shutdown flags, allowing wakeup |
| `CANID_RX_TELEMBEAT` | Updates `last_rx_telembeat` to gate telemetry transmission |
| `CANID_RX_SET_CONFIG` / `CANID_RX_GET_CONFIG` | Delegated to `HandleCanConfig()` |
| IVT-S frames (0x21–0x28) | Unpack 48-bit big-endian signed values into current/voltage/power/charge/energy |
| `CANID_ISENSE_CHARGE` | First receipt sets `q_offset` — the IVT-S charge accumulator zero reference |
| `CANID_RX_SET_INITIAL_CHARGE` | Overrides SoC initial charge from CAN (big-endian 32-bit µAh) |
| `CANID_RX_BLACKBOX_REQUEST` | Sets `blackbox.requested` for transmission next iteration |
| `CANID_ELCON_RX` | Delegates to `HandleElconHeartbeat()` |

---

## `DbmsErr()` / `DbmsClose()`

`DbmsErr()` is called on unrecoverable HAL errors. It shuts down the stack and sets the firmware fault LED. `DbmsClose()` sets the idle LED on clean exit.

---

## Interactions

| Module | Role |
|--------|------|
| All modules | Orchestrated from `DbmsIter()` |
| [Stack](stack.md) | Wakeup, volt/temp reads, fault checks |
| [Faults](faults.md) | `CheckVoltageFaults/CurrentFaults/TemperatureFaults`, `SetFaultLine`, `ThrowHardFault` |
| [Charging](charging.md) | `ChargingUpdate()` every iteration |
| [Model](model.md) | `UpdateModel()` every iteration |
| [Blackbox](blackbox.md) | `BlackboxUpdate()` every iteration; send/save on request/fault |
| [Precharge](precharge.md) | `PrechargeUpdate()` every iteration |
| [VInterface](vinterface.md) | All telemetry transmission; CAN RX config handling |
| [Sched](sched.md) | `GetUs()`, `CalcIterDelay()` for loop pacing |
| [Storage](storage.md) | Periodic saves for settings, charge stats, faults |
