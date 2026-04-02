# Blackbox

**Source:** `Core/Src/dbms/blackbox.c`, `blackbox.h`

## Overview

The Blackbox module implements a fault-triggered event recorder. It continuously captures a snapshot of the full system state into a circular in-RAM buffer on every main loop iteration. When a hard fault occurs, the current buffer contents are persisted to EEPROM for post-mortem analysis. The buffer can also be requested and transmitted over CAN at any time.

The design goal is to have a short history of what the system looked like *leading up to* a fault, not just at the instant of failure.

---

## Snapshot Structure

Every iteration, a `Snapshot` struct (defined in `context.h`) is written into the circular buffer. At ~194 bytes per snapshot, the 10-snapshot buffer provides a rolling window of roughly the last 500 ms of system state at 20 Hz.

Each snapshot captures:

| Field | Type | Description |
|-------|------|-------------|
| `iter` | `uint64_t` | Global iteration counter at time of capture |
| `current_ma` | `int32_t` | Pack current from IVT-S sensor (milliamps) |
| `voltage_mv` | `int32_t` | Pack voltage from IVT-S sensor (millivolts) |
| `qd` | `float` | Charge delta since last save (accumulated loss) |
| `current_limit_a` | `float` | Computed current limit from model |
| `dcir` | `float` | DC internal resistance estimate |
| `total_resistance` | `float` | Total ECM resistance (max of DCIR and RC model) |
| `avg_temp` | `float` | Average cell temperature (°C) |
| `max_temp` | `float` | Maximum cell temperature (°C) |
| `min_temp` | `float` | Minimum cell temperature (°C) |
| `cell_delta_v` | `float` | max_voltage − min_voltage across all groups (V) |
| `high_voltage` | `float` | Maximum cell group voltage (V) |
| `low_voltage` | `float` | Minimum cell group voltage (V) |
| `avg_voltage` | `float` | Average cell group voltage (V) |
| `controller_mask` | `uint32_t` | Active fault bitmask (see [Faults](faults.md)) |
| `bridge_fault_summary` | `uint8_t` | BQ79600 bridge fault summary byte |
| `bridge_faults` | `uint32_t` | Full bridge fault register |
| `monitor_fault_summary` | `uint8_t[N_MONITORS]` | Per-monitor fault summary bytes |

---

## Circular Buffer Design

The buffer metadata is tracked in the `Blackbox` struct (in `DbmsCtx`):

```c
typedef struct _Blackbox {
    uint8_t head;       // Index of oldest snapshot
    uint8_t count;      // Number of valid snapshots currently stored (max 10)
    bool    requested;  // Set by CAN request; triggers CAN transmission
    bool    ready;      // Set when EEPROM save is complete
} Blackbox;
```

- Buffer capacity: 10 snapshots
- On every iteration: write current state to `snapshots[head]`, advance `head`, increment `count` (capped at 10)
- The result is a rolling window: once full, the oldest snapshot is overwritten
- Snapshot data is stored in EEPROM starting at page 3 (address `0x300`), with metadata (head/count) at page 2 (`0x200`)

---

## Fault Trigger

When `ThrowHardFault()` is called from the Faults module, it sets `flags.need_to_save_blackbox = true`. The main loop detects this flag and calls `BlackboxSave()`, which writes all current snapshots to EEPROM in one batch. This preserves the pre-fault history.

---

## CAN Transmission

A remote node (e.g., the DBMS Windows app) can request the blackbox over CAN. When the request arrives:
1. `blackbox.requested` is set to `true`
2. On the next iteration, `BlackboxSend()` is called
3. Each snapshot is transmitted as a series of 6-byte CAN frames using extended IDs in the range `0x0B007000`–`0x0B009000`
4. The CAN frames are sent sequentially; the receiver reassembles them

---

## EEPROM Memory Map

| Address | Content |
|---------|---------|
| `0x200` | Blackbox metadata (head, count) |
| `0x300` | Snapshot 0 (100 bytes) |
| `0x364` | Snapshot 1 |
| ... | ... |
| Snapshot N | `0x300 + N * 100` |

> The on-disk snapshot format may be a compact/truncated version of the full in-RAM struct. See `storage.c` for the layout.

---

## Interactions

| Module | Interaction |
|--------|-------------|
| [DBMS](dbms.md) | Calls `BlackboxUpdate()` each iteration; calls `BlackboxSend()` when requested |
| [Faults](faults.md) | Calls `ThrowHardFault()` which triggers save flag |
| [Storage](storage.md) | `BlackboxSave()` writes to EEPROM via `SaveStoredObject()` |
| [CAN](can.md) | Blackbox request received via CAN RX; transmission via CAN TX queue |
| [VInterface](vinterface.md) | Dispatches the blackbox CAN request from the RX handler |
