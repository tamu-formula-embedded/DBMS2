# Model

**Source:** `Core/Src/dbms/model/model.c`, `model.h`, `data.c`, `data.h`

## Overview

The model module implements a dual-path Equivalent Circuit Model (ECM) for the battery pack. Each iteration it estimates state of charge (SoC) via two independent paths — an open-circuit voltage path and an RC-circuit path — computes DC internal resistance, and derives a current limit based on the minimum cell voltage and the worst-case resistance estimate.

---

## Battery Model Architecture

The model operates on a per-cell basis but receives pack-level inputs from the IVT-S sensor. The input current is divided by `N_P_GROUP` (3 parallel cells per group) to get the per-cell current, and the pack voltage is divided by `N_SIDES × N_GROUPS_PER_SIDE` (52 series groups) to get the per-cell voltage.

### OC Path (Open-Circuit SoC)

The OC path estimates SoC by tracking cumulative charge loss from the pack and looking up the corresponding open-circuit voltage from a temperature-interpolated LUT.

1. The remaining charge `Q` is `Q_initial − Q_discharged`, where `Q_discharged` comes from the IVT-S charge accumulator (converted to Ah and divided by N_P_GROUP)
2. `Q_max` is temperature-interpolated between two calibrated capacity bounds (`Q_BOUND_L_OC = 4.3696 Ah` at 45°C and `Q_BOUND_H_OC = 4.3967 Ah` at 25°C)
3. `z_oc = Q / Q_max` — fractional SoC
4. `V_oc` is looked up from a 201-entry OCV table at temperature

If current is flowing, DCIR is computed as `R_oc = (V_oc − V_pack) / I`, where `V_pack` is the measured per-cell voltage from the IVT-S.

### RC/ECM Path

The RC path uses the same charge-tracking approach but with different capacity bounds (`Q_BOUND_L_RC`, `Q_BOUND_H_RC`), yielding a second SoC estimate `z_rc`. From this, three resistance values (R0, R1, R2) are looked up from separate temperature-interpolated LUTs representing the ECM's series resistances. Their sum `R_rc = R0 + R1 + R2` is the total ECM resistance.

### Current Limit

`R_cell = max(R_oc, R_rc)` takes the worst-case resistance from either path. The current limit is then:

```
I_lim = ((V_min_static − V_dyn_min) / R_cell) × N_P_GROUP
```

where `V_min_static` is the lowest observed cell voltage from the stack, and `V_dyn_min` is a configurable floor voltage (`DYNAMIC_V_MIN`, default 2600 mV) representing the minimum allowed cell voltage under load. This gives the headroom available before any cell would dip below that floor.

---

## LUT Structure and Interpolation

All model LUTs are dual-temperature: there is a "low temperature" array (25°C calibration) and a "high temperature" array (45°C calibration) for each quantity. `LookupFromZ()` performs bilinear interpolation: first interpolating within each temperature array at the given SoC index, then interpolating between the two temperature results at the actual average pack temperature.

The OCV LUT has 201 entries spanning SoC from 0 to 1, where index 0 corresponds to SoC = 1.0 (full) and index 200 corresponds to SoC = 0.0 (empty). The RC resistance LUTs each have 101 entries with the same SoC indexing. This reversed ordering (high SoC at low index) is a consequence of how the calibration data was generated and is handled inside `LookupFromZ()`.

---

## Thermistor LUT

The thermistor voltage-to-temperature conversion uses a separate LUT built at startup. `DataInit()` calls `lut_build()` to sort the `(voltage, temperature)` pairs and store them in `ctx->data.lut_therm_v_to_t`. The LUT covers 0–120°C in 1°C steps with corresponding thermistor voltage values. `ThermVoltToTemp()` performs a binary-search interpolated lookup each time a temperature reading is converted.

---

## Charge Stats Persistence

`qstats.initial` is the user-supplied initial SoC (set via CAN), in Ah. `qstats.accumulated_loss` is the charge consumed this session from the IVT-S accumulator. `qstats.historic_accumulated_loss` is the carry-over from previous sessions, stored to EEPROM.

On every wakeup, `historic_accumulated_loss` is loaded from EEPROM and used as the starting point for total loss. Every 400 iterations (~20 seconds), `PeriodicSaveQStats()` writes the updated total back to EEPROM — but only if it has changed by more than 0.1 Ah to avoid unnecessary wear.

---

## Interactions

`UpdateModel()` is called from `DbmsIter()` every iteration. It reads from `ctx->current_sensor` (IVT-S data), `ctx->stats` (min/avg voltage/temperature), and `ctx->qstats` (charge tracking). Results are written to `ctx->model` and broadcast over CAN via [VInterface](vinterface.md) metrics 20–32.
