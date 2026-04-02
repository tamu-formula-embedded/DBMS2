# Settings

**Source:** `Core/Src/dbms/settings.c`, `settings.h`

## Overview

The settings system stores all user-configurable parameters as an array of `uint32_t` values in a `DbmsSettings` struct. Settings are indexed by the `UserSettingIndex` enum, persisted to EEPROM page 0, and accessible over CAN via the CFG_SET/CFG_GET protocol.

---

## Settings Reference

All values are stored as `uint32_t`. Voltage values are in millivolts, temperatures in degrees Celsius, times in milliseconds, and currents in amps unless noted.

| Index | Name | Default | Notes |
|-------|------|---------|-------|
| 1 | `QUIET_MS_BEFORE_SHUTDOWN` | 2000 | Heartbeat watchdog timeout (ms) |
| 2 | `MAX_GROUP_VOLTAGE` | 4200 | Per-group overvoltage threshold (mV) |
| 3 | `MIN_GROUP_VOLTAGE` | 2500 | Per-group undervoltage threshold (mV) |
| 4 | `MAX_PACK_VOLTAGE` | 600000 | Pack overvoltage (mV) |
| 5 | `MIN_PACK_VOLTAGE` | 30000 | Pack undervoltage (mV) |
| 6 | `MAX_THERMISTOR_TEMP` | 45 | Overtemperature fault threshold (°C) |
| 7 | `MAX_CURRENT` | 50 | Max continuous current at low temp (A) |
| 8 | `MAX_V_DELTA` | 100 | Max allowed cell voltage spread (mV) |
| 9 | `DYNAMIC_V_MIN` | 2600 | Minimum cell voltage under load for I_lim calc (mV) |
| 10 | `MS_BEFORE_FAULT_CHECKS` | 10000 | Startup delay before fault checks begin (ms) |
| 11 | `PULSE_LIMIT_CURRENT` | 90 | Current pulse fault threshold (A) |
| 12 | `PULSE_LIMIT_TIME_MS` | 3000 | Duration before current pulse fault (ms) |
| 13 | `OVERTEMP_MS` | 1000 | Sustained overtemp duration to throw fault (ms) |
| 14 | `TEMP_CURVE_A` | 50 | Temperature below which full current limit applies (°C) |
| 15 | `TEMP_CURVE_B` | 65 | Temperature above which current limit is zero (°C) |
| 16 | `IGNORE_BAD_THERMS` | 1 | Replace out-of-range thermistor readings with pack average |
| 17 | `CH_BAL_DELTA_BEGIN` | 200 | Cell voltage spread to start balancing (mV) |
| 18 | `CH_BAL_DELTA_END` | 150 | Cell voltage spread to stop balancing (mV) |
| 19 | `CH_BAL_MIN_V` | 3800 | Minimum cell voltage to permit balancing (mV, unused) |
| 20 | `CH_BAL_T_IDX` | 2 | Balance timer index: 0=0s, 1=10s, 2=30s, 3=60s |
| 21 | `CH_TARGET_V` | 4200 | Per-cell charge target voltage (mV) |
| 22 | `CH_I` | 18 | Max charge current (A) |
| 23 | `CH_AC_VOLTAGE` | 240 | AC supply voltage for power calculation (V) |
| 24 | `CH_ELCON_EFF` | 90 | Elcon charger efficiency (%) |
| 25 | `FAN_T_TH` | 30 | Fan turn-on temperature threshold (°C) |
| 26 | `FAN_DUTY` | 100 | Fan duty cycle (%, unused — always 100%) |
| 27 | `LOW_PLAUSIBLE_TEMP` | 10 | Low plausibility bound for thermistor readings (°C) |
| 28 | `HIGH_PLAUSIBLE_TEMP` | 120 | High plausibility bound for thermistor readings (°C) |
| 29 | `PRECHARGE_ON_TH` | 90 | Precharge engage threshold (%, unused) |
| 30 | `PRECHARGE_OFF_TH` | 10 | Precharge disengage threshold (%, unused) |

Index 0 (`__UNUSED`) is always skipped. The enum ends with `__NUM_USER_DEFINED_SETTINGS` which determines the array size at compile time.

---

## EEPROM Storage

Settings are stored as a raw `DbmsSettings` struct at EEPROM address `0x000` (page 0) with a CRC16 trailer appended by `SaveStoredObject()`. On startup, if the CRC check fails (e.g., first boot or struct layout change), `LoadFallbackSettings()` initializes all fields to the defaults listed above and schedules an EEPROM write.

---

## CAN Access

Settings are readable and writable over CAN via the CFG_GET (0x0B6 RX) and CFG_SET (0x0B5) protocol described in [CAN](can.md). The setting index is passed as a byte, and the value as a 4-byte big-endian integer. On a CFG_SET, `need_to_sync_settings` is set so the DBMS saves to EEPROM on the next iteration.

The DBMS heartbeat frame broadcasts a CRC16 of the entire settings struct each iteration, allowing a PC-side tool to detect settings changes without polling.

---

## Interactions

`GetSetting()` and `SetSetting()` are called from virtually every module. [Storage](storage.md) handles the EEPROM persistence. [VInterface](vinterface.md) handles the CAN protocol layer for CFG_SET/GET.
