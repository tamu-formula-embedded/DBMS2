# Charging

**Source:** `Core/Src/dbms/charging.c`, `charging.h`

## Overview

The charging module implements the state machine that controls the full charging cycle: detecting a J1772 connection, commanding the Elcon charger, waiting for cell voltages to settle, and running interleaved odd/even cell balancing. It is called from `DbmsIter()` unconditionally every iteration.

---

## State Machine

```
         ┌─────────────────────────────────────────────┐
         │  disconnected at any time → CH_NO_CONN      │
         └─────────────────────────────────────────────┘

CH_NO_CONN ──(connected)──► CH_CHARGING
                                │
                        (NeedsToBalance after 1s)
                                │
                            CH_WAIT_1  ──(10 s settle)──► CH_BALANCING_EVENS
                                                                │
                                                         CH_BALANCING_ODDS
                                                                │
                                                            CH_WAIT_2  ──(10 s settle)──►
                                                                │
                                              NeedsToBalanceMore? ──yes──► CH_BALANCING_EVENS
                                                                │
                                                              no
                                                                │
                                                         CH_CHARGING (loop)
```

`CH_COMPLETE` exists in the enum but is not reached in the current code — there is no transition into it from the active cycle.

---

## Connection Detection

`ChargingConnected()` is true when:
1. `flags.active` is set (system is awake)
2. `ElconConnected()` — Elcon heartbeat received within `QUIET_MS_BEFORE_SHUTDOWN`
3. `j1772.pp_connect` — PP pilot ADC voltage below 1600 mV

If any condition fails, the state machine immediately drops to `CH_NO_CONN` regardless of current state. This is checked unconditionally at the top of `ChargingUpdate()` before the main switch.

---

## Charge Request Calculation

In `CH_CHARGING`, the Elcon voltage/current request is computed each iteration:

- **Voltage target:** `CH_TARGET_V × N_GROUPS_PER_SIDE × N_SIDES`, clamped to 600 V
- **Current:** minimum of `j1772.maxCurrentSupply` (from CP duty cycle) and `CH_I` setting, then scaled by AC voltage and Elcon efficiency

This means the charger is power-limited by the J1772 EVSE's advertised capability via the CP PWM duty cycle.

---

## Balance Trigger Logic

`NeedsToBalance()` is checked after the system has been in `CH_CHARGING` for at least 1 second. It returns true if either:
- `max_v − min_v > CH_BAL_DELTA_BEGIN` (voltage spread is wide), **or**
- `max_v` is at the target voltage AND the spread is still above `CH_BAL_DELTA_END`

The two thresholds (`CH_BAL_DELTA_BEGIN` default 200 mV, `CH_BAL_DELTA_END` default 150 mV) create hysteresis: balancing starts wider than it stops.

---

## Pre-Balance Voltage Accumulation

During `CH_WAIT_1` and `CH_WAIT_2`, the charger is disabled (Elcon commanded to 0 V/0 A) and the system waits 10 seconds for voltages to settle. After the first second, voltages are accumulated per-cell and averaged (`ChargingAccumulateVoltages` + `ChargingComputePreBalanceAverages`).

These per-cell **pre-balance averages** are what `StackComputeCellsToBalance()` uses — not the live readings — so balancing decisions are based on the open-circuit resting voltage, not transient charge-pump artifacts.

`pre_bal_min_v` is the minimum voltage seen across all cells in the averaged window; this becomes the reference used by balancing threshold checks.

---

## Interleaved Odd/Even Balancing

The BQ79616 only allows one cell's bypass switch active at a time on adjacent cells (to avoid creating a short). Balancing always runs evens first, then odds. Within each pass:

1. `StackComputeCellsToBalance()` marks cells needing balancing (those whose pre-balance average exceeds `pre_bal_min_v + CH_BAL_DELTA_END`)
2. `StackStartBalancing()` programs the per-cell balance timers on every monitor and fires `BAL_GO`
3. `DoneBalancing()` returns true once the time index (0/10/30/60 s from `CH_BAL_T_IDX`) plus 6 s overhead has elapsed
4. If no cells actually need balancing (`StackNeedsToBalance()` returns false), the phase is skipped immediately

After odds complete, the system goes to `CH_WAIT_2`. If `NeedsToBalanceMore()` is true (spread of pre-balance min to current max still exceeds `CH_BAL_DELTA_END`), another full balance cycle is triggered. Otherwise it resumes charging.

---

## LED States During Charging

| State | LED Pattern |
|-------|------------|
| `CH_CHARGING` | `LED_CHARGING` |
| `CH_WAIT_1`, `CH_WAIT_2` | `LED_CHARGING_WAIT` |
| `CH_BALANCING_EVENS` | `LED_BALANCING_EVENS` |
| `CH_BALANCING_ODDS` | `LED_BALANCING_ODDS` |
| `CH_COMPLETE` | `LED_CHARGING_COMPLETE` |

---

## Interactions

| Module | Interaction |
|--------|-------------|
| [J1772](j1772.md) | `J1772ReadState()` called at the start of every `ChargingUpdate()` |
| [Elcon](elcon.md) | `SendElconRequest()` called every iteration — either with target V/I or zeroed to disable |
| [Stack](stack.md) | `StackComputeCellsToBalance()`, `StackStartBalancing()`, `StackNeedsToBalance()` |
| [VInterface](vinterface.md) | `SendCellsToBalance()` broadcasts the balance map over CAN every balancing iteration |
| [LedCtl](ledctl.md) | Sets `ctx->led_state` based on charging phase |
| [Settings](settings.md) | Reads `CH_TARGET_V`, `CH_BAL_DELTA_BEGIN`, `CH_BAL_DELTA_END`, `CH_BAL_T_IDX`, `CH_I`, `CH_AC_VOLTAGE`, `CH_ELCON_EFF` |
