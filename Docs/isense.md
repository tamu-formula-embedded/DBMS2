# ISense

**Source:** `Core/Src/dbms/isense.c`, `isense.h`

## Overview

The ISense module handles communication with the IVT-S current sensor from Isabellenhütte. The IVT-S measures pack current, voltages across three measurement points, power, and accumulated charge, broadcasting each on its own CAN ID at a configurable rate. This module configures the sensor on startup and provides the data unpacking function used by the CAN RX dispatch.

---

## Configuration Sequence

`ConfigCurrentSensor()` is called during `DbmsPerformWakeup()`. The IVT-S must be stopped, reconfigured, and restarted — the entire sequence is sent twice for reliability. The procedure is:

1. Send a stop-mode command (`0x34, 0, 1, ...`) to halt the sensor's output
2. For each measurement type (current, voltage 1, voltage 2, voltage 3, power), send a cycle-time configuration frame setting the measurement period in milliseconds
3. Send a run-mode command (`0x34, 1, 1, ...`) to restart measurement

All configuration commands are sent to `CANID_ISENSE_COMMAND` with a 2 ms delay between frames.

---

## Data Format

Each IVT-S measurement frame carries a 48-bit big-endian signed integer in bytes 0–5. `UnpackCurrentSensorData()` assembles these six bytes into the low 48 bits of a 64-bit value and then sign-extends from bit 47 to produce a proper `int64_t`. The units are determined by measurement type: milliamps for current, millivolts for voltage, watts for power, and amp-seconds for charge.

The charge measurement deserves special attention. The IVT-S internally accumulates charge since the last reset, so the DBMS records a `q_offset` on the first charge frame received after wakeup. All subsequent charge readings are relative to this offset, giving a clean zero-referenced accumulated charge for the current session.

---

## Interactions

Configuration is called from [DBMS](dbms.md) wakeup. All received measurement frames are dispatched from `DbmsCanRx()` — the raw decoded values are stored directly in `ctx->current_sensor`. The [Model](model.md) reads `current_sensor.current_ma`, `voltage1_mv`, and `charge_as` every iteration. The charge offset logic in `DbmsCanRx()` handles the q_offset initialization.
