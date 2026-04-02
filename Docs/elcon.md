# Elcon

**Source:** `Core/Src/dbms/elcon.c`, `elcon.h`

## Overview

The Elcon module handles the CAN protocol for the Elcon DC charger. It provides two functions: processing incoming charger status frames and sending charge requests. All charging logic (when to charge, what voltage/current to request) lives in [Charging](charging.md) — this module is purely the protocol layer.

---

## CAN Protocol

| ID | Direction | Description |
|----|-----------|-------------|
| `0x1806E5F4` | TX | Charge request (voltage target, current limit, enable/disable) |
| `0x18FF50E5` | RX | Charger status (output voltage, output current, status flags) |

Both use 29-bit extended IDs.

### TX Frame Format (`SendElconRequest`)

```
Byte 0-1: voltage request × 10  (big-endian, 0.1 V/LSB)
Byte 2-3: current request × 10  (big-endian, 0.1 A/LSB)
Byte 4:   enable (1 = charge enabled, 0 = disabled/stop)
Byte 5-7: 0x00
```

To stop charging, the frame is sent with all-zero V/I and `enable = 1` (not `enable = 0`). Sending `enable = 0` is used in `CH_NO_CONN`.

### RX Frame Format (`HandleElconHeartbeat`)

```
Byte 0-1: output voltage × 10   (big-endian)
Byte 2-3: output current × 10   (big-endian)
Byte 4:   (unused)
Byte 5:   status flags
```

On receipt, `ctx->elcon.heartbeat` is updated to the current tick. The Charging module uses this timestamp to determine if the charger is still alive (`ElconConnected()` checks against `QUIET_MS_BEFORE_SHUTDOWN`).

---

## Interactions

| Module | Interaction |
|--------|-------------|
| [Charging](charging.md) | Calls `SendElconRequest()` every iteration; checks `ElconConnected()` via heartbeat timestamp |
| [DBMS](dbms.md) | `HandleElconHeartbeat()` called from CAN RX dispatch |
| [CAN](can.md) | Transmits via `CanTransmit()` on `CANID_ELCON_TX` |
