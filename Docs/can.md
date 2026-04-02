# CAN

**Source:** `Core/Src/dbms/can.c`, `can.h`

## Overview

The CAN module manages all CAN bus communication for the DBMS controller. It provides a software TX queue to prevent blocking on bus-saturated conditions, interrupt-driven receive dispatch, and filter configuration. The controller has two physical CAN buses (CAN1 and CAN2); CAN1 is used for vehicle communication and CAN2 for the Elcon charger.

---

## TX Queue

Outgoing CAN frames are not transmitted directly — they are pushed into a 512-entry ring buffer and transmitted by an interrupt handler when the hardware mailboxes become free. This prevents the 20 Hz main loop from blocking on CAN bus contention.

| Parameter | Value |
|-----------|-------|
| TX queue depth | 512 frames |
| Drop behavior | Oldest frame dropped if queue is full (`n_tx_can_drop_queue_full` incremented) |

When a mailbox becomes free, the TX complete interrupt pops the next frame from the queue and loads it. The CAN TX FIFO has 3 hardware mailboxes.

---

## RX Filters

The hardware CAN acceptance filters are configured at init to pass only the IDs the DBMS cares about. Incoming frames are dispatched from the RX FIFO interrupt to the appropriate handler via `DbmsCanRx()` in [DBMS](dbms.md).

---

## CAN ID Map

### Standard IDs (11-bit)

| CAN ID | Direction | Description |
|--------|-----------|-------------|
| `0x0B1` | RX | Vehicle heartbeat (provides real-time clock sync) |
| `0x0B2` | TX | DBMS heartbeat (includes settings CRC) |
| `0x0B5` | RX | Config SET (write a setting) |
| `0x0B6` | TX/RX | Config GET request / response |

### Extended IDs (29-bit)

| CAN ID Range | Direction | Description |
|-------------|-----------|-------------|
| `0x0B001000`–`0x0B001FFF` | TX | Cell voltage telemetry frames |
| `0x0B002000`–`0x0B002FFF` | TX | Cell temperature telemetry frames |
| `0x0B003000` | TX | Pack metrics (current, voltage, power, charge, energy) |
| `0x0B004000` | TX | CAN log / debug string output |
| `0x0B005000` | TX | Loop timing and fault mask metrics |
| `0x0B007000`–`0x0B009000` | TX | Blackbox snapshot transmission |
| `0x1806E5F4` | TX | Elcon charger command (voltage/current request) |
| `0x18FF50E5` | RX | Elcon charger status (voltage/current feedback) |
| `0x00000021` | RX | IVT-S current measurement |
| `0x00000022` | RX | IVT-S voltage 1 measurement |
| `0x00000025` | RX | IVT-S voltage 2 measurement |
| `0x00000026` | RX | IVT-S voltage 3 measurement |
| `0x00000027` | RX | IVT-S power measurement |
| `0x00000028` | RX | IVT-S charge measurement |

---

## Statistics

The following counters in `Stats` track CAN bus health:

| Counter | Description |
|---------|-------------|
| `n_tx_can_frames` | Total frames successfully transmitted |
| `n_rx_can_frames` | Total frames received |
| `n_unmatched_can_frames` | Received frames not matching any known ID |
| `n_tx_can_drop_queue_full` | TX frames dropped due to full queue |
| `n_tx_can_fail` | TX frames that failed hardware transmission |
| `n_tx_queued` | Current number of frames in TX queue |

---

## CAN Log (`CanLog`)

`CanLog()` in [VInterface](vinterface.md) provides a `printf`-style debug logging facility over CAN. Messages are packed into 8-byte CAN frames (extended ID `0x0B004000`) with a sequential 16-bit ordering index. This allows a PC-side receiver to reconstruct the message stream. The ordering index is tracked in `DbmsCtx.can_log_ordering_index`.

---

## Heartbeat Protocol

### TX Heartbeat (`0x0B2`)
Transmitted every main loop iteration. Payload includes:
- A CRC16 of the current settings struct

This allows connected tools to detect when settings have changed.

### RX Heartbeat (`0x0B1`)
Received from the vehicle ECU. Used for:
1. **Active watchdog** — if no heartbeat is received for `SETTINGS_QUIET_TIMEOUT_MS` (default 2000 ms), the DBMS requests shutdown
2. **Real-time clock sync** — the heartbeat carries a global timestamp that is used to align the DBMS's local timer offset (`Realtime.global_ts` / `local_ts`)

---

## Config Protocol

### CFG_SET (ID `0x0B5`)
8-byte frame:
```
[ setting_index (2 bytes) | value (4 bytes) | padding (2 bytes) ]
```
On receipt, the setting is updated in RAM and `flags.need_to_sync_settings` is set, triggering an EEPROM save on the next iteration.

### CFG_GET Request (ID `0x0B6`, RX)
2-byte frame: `[ setting_index (2 bytes) ]`

### CFG_GET Response (ID `0x0B6`, TX)
6-byte frame: `[ setting_index (2 bytes) | value (4 bytes) ]`

---

## Interactions

| Module | Interaction |
|--------|-------------|
| [DBMS](dbms.md) | Calls `CanTxHeartbeat()` each loop; `DbmsCanRx()` dispatches all incoming frames |
| [VInterface](vinterface.md) | Calls `CanTx()` for all telemetry; implements `CanLog()` |
| [ISense](isense.md) | Sends configuration frames; receives measurement frames |
| [Elcon](elcon.md) | Sends charger commands; receives charger status |
| [Settings](settings.md) | CFG_SET/GET handled via CAN |
| [Blackbox](blackbox.md) | Blackbox snapshots transmitted as CAN frames |
