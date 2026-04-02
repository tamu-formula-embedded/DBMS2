# Fan

**Source:** `Core/Src/dbms/fan.c`, `fan.h`

## Overview

The fan module controls a PWM-driven cooling fan. It is conditionally compiled with `#ifdef HAS_FAN` — if that define is not set, no fan code is built and `InitFan`/`UpdateFan` calls in `dbms.c` are omitted.

---

## Behavior

`UpdateFan()` is called once per main loop iteration. Logic is binary: if `stats.avg_t` exceeds `FAN_T_TH` (default 30°C), the fan runs at 100% duty; otherwise it is off. There is no proportional or PID control — it is full-on or full-off.

The PWM output is on timer `timer_pwm_1`, channel 4. Duty cycle is set by computing the compare register value from the auto-reload register (ARR), ensuring correct scaling regardless of timer frequency.

---

## Interactions

| Module | Interaction |
|--------|-------------|
| [DBMS](dbms.md) | `InitFan()` at startup, `UpdateFan()` each iteration — both inside `#ifdef HAS_FAN` |
| [Settings](settings.md) | Reads `FAN_T_TH` for the on/off temperature threshold |
