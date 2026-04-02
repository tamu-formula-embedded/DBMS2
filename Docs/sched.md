# Sched

**Source:** `Core/Src/dbms/sched.c`, `sched.h`

## Overview

The scheduling module provides microsecond-precision timing, loop pacing, and real-time clock synchronization. It is the timing substrate that the rest of the firmware depends on.

---

## Microsecond Timer

`GetUs()` returns a monotonic 64-bit microsecond timestamp derived from a hardware timer peripheral configured to tick at 1 MHz. The STM32 hardware counter is 16 or 32 bits wide, so `GetUs()` uses a static overflow counter to extend it to 64 bits: each time the counter wraps around (detected when the current reading is less than the last reading), the high accumulator is incremented by one full counter period. This means `GetUs()` must be called at least once per overflow period to detect wraps correctly — at 1 MHz and a 16-bit counter, that is every ~65 ms, well within the 50 ms main loop cadence.

`DelayUs()` uses `GetUs()` to implement a busy-wait microsecond delay. This is used for precise inter-frame delays when communicating with the BQ79616 stack, where `HAL_Delay()` granularity (1 ms) is too coarse.

---

## Loop Pacing

`CalcIterDelay()` computes how many microseconds remain in the current 50 ms loop budget. It takes the elapsed time (`iter_end_us − iter_start_us`) and subtracts it from the target period. If the loop ran over budget, it returns 0. The DBMS main loop then calls `HAL_Delay(remaining_ms)` and `DelayUs(remaining_us % 1000)` to fill the remainder, keeping the loop at a consistent 20 Hz without accumulating drift.

---

## Real-Time Clock Synchronization

The DBMS has no RTC hardware, so it maintains a software-derived global timestamp by synchronizing against the vehicle ECU's heartbeat. When a heartbeat CAN frame arrives, `SyncRealTime()` records the remote timestamp (`global_ts`) and the local `HAL_GetTick()` at that moment (`local_ts`). `GetRealTime()` then computes the current global time as `global_ts + (HAL_GetTick() − local_ts)` — a dead-reckoned estimate that drifts slowly between sync events. At 20 Hz heartbeats, drift is negligible for logging purposes.

This real-time value is broadcast as metric 34 and used in charge stats for session timestamping.

---

## Interactions

`GetUs()` and `DelayUs()` are called throughout [Stack](stack.md) and [DBMS](dbms.md). `CalcIterDelay()` is called at the end of every `DbmsIter()`. `SyncRealTime()` and `GetRealTime()` are called from [DBMS](dbms.md) CAN RX and [VInterface](vinterface.md) respectively.
