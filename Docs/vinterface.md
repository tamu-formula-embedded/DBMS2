# VInterface

**Source:** `Core/Src/dbms/vinterface.c`, `vinterface.h`

## Overview

VInterface (Vehicle Interface) is the outward-facing CAN layer — everything the DBMS broadcasts to the rest of the vehicle or to a connected PC tool lives here. It handles telemetry packing, the two metrics systems, debug logging, heartbeat transmission, and the settings config protocol.

---

## Two Metrics Systems

There are two separate metrics broadcast pathways with different intended audiences:

**Plex Metrics (`SendPlexMetrics`)** are compact frames targeted at the vehicle ECU and the PLEX data logger. These use standard 11-bit CAN IDs in the 0x10–0x1F range. The payload format is fixed and minimal: SoC as a byte (0–100), fault mask as a 32-bit integer, pack current in amps, etc. These are broadcast unconditionally every iteration regardless of whether a PC tool is connected.

**Full Metrics (`SendMetrics`)** are an extended dump of ~85 individual values, each packed as a 64-bit big-endian integer in an 8-byte frame using extended ID `0x0B005xxx`. The lower 12 bits of the ID are the metric index. This includes everything: IVT-S data, model outputs, fault details, timing profiling checkpoints, Elcon status, J1772 status, charging state, and per-monitor CRC error rates. These are only transmitted when `telem_enable` is true (a telembeat frame received within the last 5 seconds), to avoid flooding the bus when no PC tool is present.

---

## Cell Voltage and Temperature Telemetry

`SendCellVoltages()` and `SendCellTemps()` pack per-cell data into extended-ID frames under `CANID_CELLSTATE_VOLTS` (`0x0B001xxx`) and `CANID_CELLSTATE_TEMPS` (`0x0B002xxx`). The lower bits of the ID encode the side number and chunk offset, allowing a receiver to reassemble all 13 groups per side from sequential frames. Values are 16-bit big-endian integers: voltage in 0.1 mV units, temperature in millidegrees C. `SendCellsToBalance()` sends the balance mask in the same format.

The packing helper `SendCellDataBuffer()` walks through the per-side buffer in chunks of 4 values (8 bytes) and transmits each chunk with the appropriate composite CAN ID.

---

## CanLog — Debug Logging Over CAN

`CanLog()` is a `printf`-style debug logging function that fragments formatted strings into 8-byte CAN frames using extended ID `CANID_CONSOLE_C0` (`0x0B004xxx`). The lower 12 bits of the ID are a monotonically incrementing frame counter (`n_logging_frames`), which allows a PC receiver to detect dropped frames and reconstruct the message stream in order. The function is gated on `telem_enable` — if no PC tool is active, log calls are no-ops to avoid unnecessary bus traffic.

---

## Heartbeat and Config Protocol

`CanTxHeartbeat()` transmits a frame on `0x0B2` each iteration. Beyond the settings CRC in bytes 6–7, it also includes the topology constants (`N_SEGMENTS`, `N_SIDES_PER_SEG`, `N_GROUPS_PER_SIDE`, `N_TEMPS_PER_SIDE`) in the first four bytes, allowing a newly connected PC tool to discover the pack configuration without hardcoding it.

`HandleCanConfig()` processes both CFG_SET and CFG_GET actions. The setting index is in byte 0 of the frame and the value is in bytes 4–7 (big-endian). For GET, a response frame is sent immediately on the same ID (`0x0B6`). For SET, the value is applied via `SetSetting()` and `need_to_sync_settings` is flagged.

---

## Interactions

`SendPlexMetrics()` is called unconditionally every iteration from [DBMS](dbms.md). `SendMetrics()`, `SendCellVoltages()`, and `SendCellTemps()` are gated on `telem_enable`. `CanLog()` is called freely from nearly every module. `HandleCanConfig()` is called from [DBMS](dbms.md) CAN RX dispatch.
