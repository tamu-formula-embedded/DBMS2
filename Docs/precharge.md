# Precharge

**Source:** `Core/Src/dbms/precharge.c`, `precharge.h`

## Overview

The precharge module controls the precharge relay (GPIO PB4) that signals the motor controller that the accumulator is ready to connect. Its job is to close the relay only when the measured bus voltage is close enough to the pack voltage that connecting the full accumulator will not cause a damaging inrush current.

---

## Logic

`PrechargeUpdate()` is called every main loop iteration. The relay closes (`precharged = true`) only when all three conditions are simultaneously met:

1. The pack voltage computed from cell readings is above 50 V (confirming the cells are actually measured and sane, not a dead-pack or disconnected condition)
2. The IVT-S voltage measurement (`voltage1_mv`) is at least 90% of the calculated pack voltage — meaning the bus capacitors are nearly charged to pack potential
3. The fault line is not currently asserted — no active fault condition

`PrechargeSet()` directly writes the `precharged` boolean to GPIO PB4.

The 90% threshold is hardcoded in the current implementation; the settings `PRECHARGE_ON_TH` and `PRECHARGE_OFF_TH` exist in the settings system but are commented out and unused.

---

## Interactions

`PrechargeUpdate()` is called from `DbmsIter()` after fault processing. It reads from `ctx->current_sensor.voltage1_mv`, `ctx->stats.pack_v`, and `ctx->stats.fault_line_faulted`. The precharge state is broadcast as metric 70 via [VInterface](vinterface.md).
