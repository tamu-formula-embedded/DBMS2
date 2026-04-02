# Faults

**Source:** `Core/Src/dbms/faults/faults.c`, `faults.h`, `faultcheck.c`

## Overview

The fault system is responsible for detecting out-of-range conditions, asserting the hardware fault line to the SDC, and persisting fault state across power cycles. All controller faults are represented as bits in a single 32-bit mask (`faults.controller_mask`), making the current fault state immediately inspectable as a single value.

---

## Fault Types

There are 11 defined controller fault types, each occupying one bit in `controller_mask`:

- **CTRL_FAULT_VOLTAGE_OVER** (bit 0) — any cell group above `MAX_GROUP_VOLTAGE`
- **CTRL_FAULT_VOLTAGE_UNDER** (bit 1) — any cell group below `MIN_GROUP_VOLTAGE`
- **CTRL_FAULT_TEMP_OVER** (bit 2) — pack temperature sustained above limit
- **CTRL_FAULT_TEMP_UNDER** (bit 3) — (defined but not currently checked)
- **CTRL_FAULT_CURRENT_OVER** (bit 4) — instantaneous current above temperature-derated limit
- **CTRL_FAULT_CURRENT_UNDER** (bit 5) — (defined but not currently checked)
- **CTRL_FAULT_PACK_VOLTAGE_OVER** (bit 6) — total pack voltage above `MAX_PACK_VOLTAGE`
- **CTRL_FAULT_PACK_VOLTAGE_UNDER** (bit 7) — total pack voltage below `MIN_PACK_VOLTAGE`
- **CTRL_FAULT_MAX_DELTA_EXCEEDED** (bit 8) — max−min voltage across all groups exceeds `MAX_V_DELTA`
- **CTRL_FAULT_STACK_FAULT** (bit 9) — fault propagated from the BQ79616 stack (reserved)
- **CTRL_FAULT_CURRENT_PULSE** (bit 10) — current exceeded `PULSE_LIMIT_CURRENT` for longer than `PULSE_LIMIT_TIME_MS`

---

## Fault Detection

Fault checks are called from `DbmsHandleActive()` after a startup delay (`MS_BEFORE_FAULT_CHECKS`, default 10 seconds). This delay prevents false faults during the startup transient while the BQ79616 ADCs settle and the IVT-S sensor synchronizes.

**Voltage faults** (`CheckVoltageFaults`) compare `stats.max_v` and `stats.min_v` against the per-group thresholds, and also check the cell delta.

**Temperature faults** (`CheckTemperatureFaults`) use a sustained-overtemperature mechanism: if `stats.max_t` drops below the limit, `overtemp_last_ok_ts` is reset. A fault is only thrown if the temperature has been continuously above the limit for longer than `OVERTEMP_MS` (default 1 second). This prevents brief thermal spikes from latching a fault.

**Current faults** (`CheckCurrentFaults`) implement two separate checks. First, there is a temperature-derated continuous current limit: at temperatures between `TEMP_CURVE_A` and `TEMP_CURVE_B`, the max current linearly ramps from `MAX_CURRENT` down to zero. Below `TEMP_CURVE_A` the full limit applies; above `TEMP_CURVE_B` the limit is zero. Second, there is a pulse current limit (`CTRL_FAULT_CURRENT_PULSE`) for high-current transients — the current must exceed `PULSE_LIMIT_CURRENT` for more than `PULSE_LIMIT_TIME_MS` consecutively before a fault is thrown. Each time the current drops back below the limit, `pl_last_ok_ts` resets.

---

## Hardware Fault Line

`SetFaultLine()` writes to GPIO PC11, which is directly wired into the vehicle Shutdown Circuit (SDC). The SDC controls the AIR coils — pulling this line low opens the AIRs and disconnects the tractive system. The GPIO is active-low (writing `true` to faulted drives the pin low).

`ThrowHardFault()` is called at the end of every `DbmsHandleActive()`. It updates the fault line based on the current mask, and — on the first iteration that a fault appears — sets `need_to_save_faults` and `need_to_save_blackbox` to trigger EEPROM persistence and blackbox capture on the next main loop iteration.

---

## Fault Persistence

The `controller_mask` is saved to EEPROM at address `0x100` (page 1) whenever it changes. On startup, it is reloaded so that latched faults survive a power cycle. The CRC of the stored fault state is verified on load; a mismatch is logged but does not prevent startup.

Faults are cleared either remotely via a `CANID_RX_CLEAR_FAULTS` CAN frame (sets `flags.req_fault_clear`), or from the DBMS Windows app. Clearing is handled at the top of `DbmsHandleActive()` on the next iteration.

---

## Interactions

The fault module is called from [DBMS](dbms.md) each active iteration. [Blackbox](blackbox.md) is triggered by `ThrowHardFault()` setting `need_to_save_blackbox`. [Storage](storage.md) is used for both save and load. The fault mask is broadcast every iteration via [VInterface](vinterface.md) as metric 15, and also included in [Blackbox](blackbox.md) snapshots.
