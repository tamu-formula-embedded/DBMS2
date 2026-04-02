# DBMS2 Firmware Documentation

**Texas A&M University Formula SAE Electric — Distributed Battery Management System**

The DBMS is a custom battery management system designed to replace the commercial Orion BMS 2 in the AME Formula SAE Electric vehicle. It runs on an STM32F446RE microcontroller (the *Control Board / BMU*) and communicates with a distributed stack of TI BQ79616 cell monitor chips via a TI BQ79600 bridge chip.

The firmware is written in C, targeting the STM32 HAL platform, and is approximately 10,000 lines. Code reliability is the top priority — guidelines are derived from NASA's Power of 10 rules, with a provably deterministic control flow graph and bounded memory footprint. The firmware has been reviewed by a Tesla BMS team.

---

## System Architecture

```
                    ┌─────────────────────────────────┐
                    │         Control Board (BMU)      │
                    │          STM32F446RE              │
                    │                                  │
  Vehicle CAN ──────┤  CAN1/CAN2                       │
  J1772 Inlet ──────┤  J1772 Circuit  (CP/PP ADC/PWM) │
  Elcon Charger ────┤  (via CAN)                       │
  IVT-S Sensor ─────┤  (via CAN)                       │
  EEPROM ───────────┤  I2C (24CSM01)                   │
  BMS Fault Line ───┤  GPIO PC11 (to SDC)              │
  Precharge Relay ──┤  GPIO PB4                        │
  LEDs (3x R/G) ────┤  GPIO                            │
                    │                                  │
                    │  UART ──── BQ79600 (Bridge Chip) │
                    └─────────────────────────────────┘
                                    │
                              TI COM Bus (isoSPI-equivalent)
                    ┌───────────────┼───────────────────┐
              ┌─────┴────┐    ┌─────┴────┐         ┌────┴──────┐
              │ Monitor 0│    │ Monitor 1│  . . .   │ Monitor 3 │
              │ BQ79616  │    │ BQ79616  │          │ BQ79616   │
              │ Seg 0/A  │    │ Seg 0/B  │          │ Seg 1/B   │
              └──────────┘    └──────────┘          └───────────┘
                  │                │                      │
           [13 cell groups]  [13 cell groups]       [13 cell groups]
```

The control board firmware runs at **20 Hz** (50 ms loop). Each iteration it reads voltages and temperatures from the cell monitors over the COM bus, runs fault checks, updates the battery model, controls charging, manages the precharge relay, broadcasts telemetry over CAN, and updates the status LEDs.

---

## Battery Pack Topology

| Parameter | Value |
|-----------|-------|
| Cell | Reliance RS50 Li-ion |
| Pack topology | 5 structural segments, 26S 3P configuration |
| Monitor chips | 4× TI BQ79616 (N_SEGMENTS=2 × N_SIDES_PER_SEG=2) |
| Voltage groups per monitor | 13 |
| Temperature sensors per monitor | 13 (multiplexed via GPIO MUX) |
| Bridge chip | TI BQ79600 |
| MCU | STM32F446RE |
| Max pack voltage fault | 600 V |
| Cell target voltage | 4200 mV |
| Cell minimum voltage | 2500 mV |

> **Note:** `N_SEGMENTS=2` and `N_SIDES_PER_SEG=2` in code describe the COM bus daisy-chain topology (2 chains with 2 monitor chips each = 4 monitors total), not the count of physical battery structural segments (5).

---

## Glossary

| Term | Definition |
|------|-----------|
| **DBMS** | Distributed Battery Management System |
| **BMU** | Battery Management Unit — the control board running this firmware |
| **CMU** | Cell Management Unit — a monitor board with BQ79616 chip(s) |
| **COM Bus** | TI-proprietary isoSPI-equivalent differential bus used by BQ796xx chips |
| **Pack** | The full vehicle battery including all electronics |
| **Segment** | A structural sub-unit of the battery containing cells, fuse bar, bus bar, thermistors, and connectors |
| **Parallel Group** | A set of parallel-connected cells sharing one voltage reading. Also called a "group" |
| **Monitor Chip** | TI BQ79616 — measures voltages, temperatures, and performs cell balancing |
| **Bridge Chip** | TI BQ79600 — translates STM32 UART to the TI COM bus |
| **Stack** | All monitor chips in the pack connected by the COM bus |
| **GLV** | Grounded Low Voltage — electronics running on <60 V, grounded to chassis |
| **TS** | Tractive System — pack, motor controller, motor, plus safety circuits |
| **SDC** | Shutdown Circuit — safety loop directly powering the AIR coils |
| **AIR** | Accumulator Isolation Relay — disconnects TS+ and TS- from the vehicle |
| **SoC / SoX** | State of Charge / State of X (X = C: Charge, H: Health, P: Power) |
| **ECM** | Equivalent Circuit Model — the battery model used for SoC and resistance estimation |
| **DCIR** | DC Internal Resistance — estimated from (V_oc − V_pack) / I |
| **OCV** | Open Circuit Voltage — voltage of a cell/pack at rest with no load |
| **EVSE** | Electric Vehicle Supply Equipment — the charging station |

---

## Module Documentation

| Module | Source File(s) | Description |
|--------|---------------|-------------|
| [DBMS](dbms.md) | `dbms.c/h` | Top-level state machine and main loop orchestration |
| [Context](context.md) | `context.h` | Global context struct — all shared system state |
| [Stack](stack.md) | `stack.c/h` | Cell monitor comms, voltage/temp reading, balancing control |
| [Bridge](bridge.md) | `bridge.c/h` | UART frame construction/parsing layer to the BQ79600 |
| [CAN](can.md) | `can.c/h` | CAN bus TX queue, RX dispatch, interrupt handling |
| [VInterface](vinterface.md) | `vinterface.c/h` | Vehicle CAN interface — telemetry, metrics, logging, config |
| [Faults](faults.md) | `faults/faults.c/h`, `faultcheck.c` | Fault detection, fault line control, EEPROM persistence |
| [Model](model.md) | `model/model.c/h`, `model/data.c/h` | ECM battery model, SoC estimation, current limit calculation |
| [Charging](charging.md) | `charging.c/h` | Charging state machine — J1772 detection → Elcon control → balancing |
| [Precharge](precharge.md) | `precharge.c/h` | Precharge relay control for soft accumulator connect |
| [ISense](isense.md) | `isense.c/h` | IVT-S current sensor CAN configuration and data decoding |
| [Elcon](elcon.md) | `elcon.c/h` | Elcon DC charger CAN protocol interface |
| [J1772](j1772.md) | `j1772.c/h` | SAE J1772 EV charging inlet (PP/CP) interface |
| [Blackbox](blackbox.md) | `blackbox.c/h` | Circular event log with fault-triggered EEPROM persistence |
| [Settings](settings.md) | `settings.c/h` | Persistent configuration with EEPROM storage and CAN access |
| [Storage](storage.md) | `storage.c/h` | EEPROM I2C driver with CRC16 validation |
| [Sched](sched.md) | `sched.c/h` | Loop scheduling, microsecond timing, real-time clock sync |
| [LedCtl](ledctl.md) | `ledctl.c/h` | RGB LED state indication |
| [Fan](fan.md) | `fan.c/h` | Cooling fan PWM control (compile-time optional: `HAS_FAN`) |
| [Utils](utils.md) | `utils/crc.c/h`, `utils/eswap.c/h`, `utils/lut.c/h`, `utils/ma.c/h`, `utils/common.h` | CRC16, endian swap, lookup tables with interpolation, moving average |

---

## Key Thresholds (Defaults)

| Parameter | Default Value | Where Defined |
|-----------|--------------|---------------|
| Loop rate | 20 Hz (50 ms) | `context.h` |
| Cell overvoltage fault | 4200 mV | Settings |
| Cell undervoltage fault | 2500 mV | Settings |
| Pack overvoltage fault | 600 V | Settings |
| Pack undervoltage fault | 30 V | Settings |
| Overtemperature fault | 45°C (sustained >1 s) | Settings |
| Max current @ 25°C | ~50 A | Settings (temp derating) |
| Current pulse fault | >90 A for >3 s | Settings |
| Cell delta fault | >100 mV (max−min) | Settings |
| Charging target voltage | 4200 mV/cell | Settings |
| Balance start delta | 200 mV | Settings |
| Balance stop delta | 150 mV | Settings |
| Precharge threshold | 90% (IVT V / pack V) | `precharge.c` |
| J1772 PP connect threshold | <1600 mV ADC reading | `j1772.h` |
| Quiet shutdown timeout | 2000 ms (no heartbeat) | Settings |
| Fault check delay after wake | 10 s | `faults/faultcheck.c` |

---

## High-Level Data Flow

```
CAN RX ISR ──► DbmsCanRx()
                   │
                   ├─ Heartbeat → real-time clock sync, active watchdog reset
                   ├─ CFG_SET/GET → settings update + EEPROM save flag
                   ├─ IVT-S frames → current/voltage/charge data
                   ├─ Elcon frame → charger feedback
                   └─ Blackbox request → transmit flag

DbmsIter() ── called at 20 Hz ────────────────────────────────────────────────
  │
  ├─ [if blackbox requested] BlackboxSend() over CAN
  ├─ [if settings dirty] SaveSettings() to EEPROM
  ├─ [every 20 s] SaveQStats() to EEPROM
  ├─ CanTxHeartbeat() — broadcast settings CRC
  │
  ├─ Watchdog: if no heartbeat for 2 s → request shutdown
  │
  ├─ [ACTIVE] DbmsHandleActive()
  │    ├─ StackUpdateAllVoltReadings()   → COM bus poll all 4 monitors
  │    ├─ StackUpdateAllTempReadings()   → COM bus poll, mux channel rotated
  │    ├─ CheckVoltageFaults()
  │    ├─ CheckTemperatureFaults()
  │    ├─ CheckCurrentFaults()
  │    └─ SetFaultLine() → GPIO PC11
  │
  ├─ UpdateModel()  → ECM (OCV/RC paths) + DCIR → I_lim
  ├─ ChargingUpdate() → J1772 state + Elcon commands + balancing
  ├─ BlackboxUpdate() → circular buffer capture
  ├─ PrechargeUpdate() → relay GPIO PB4
  ├─ SendMetrics() / SendCellVoltages() / SendCellTemps()  → CAN TX
  └─ ProcessLedAction() / MonitorLedBlink()
```

---

## Build Configuration (compile-time defines in `context.h`)

| Define | Default | Effect |
|--------|---------|--------|
| `ITER_TARGET_HZ` | `20` | Main loop target frequency |
| `SPLIT_STACK_OPS` | `1` | Spread voltage/temp reads across alternating iterations |
| `N_SEGMENTS` | `2` | Number of COM bus daisy-chain segments |
| `N_SIDES_PER_SEG` | `2` | Monitor chips per segment |
| `N_GROUPS_PER_SIDE` | `13` | Voltage-measurable parallel groups per monitor chip |
| `N_TEMPS_PER_MONITOR` | `13` | Temperature sensors per monitor chip |
| `N_P_GROUP` | `3` | Cells in parallel per group |
| `HAS_FAN` | *(undefined)* | Enable cooling fan PWM control |
| `DEBUG_DO_OVERWRITE_TEMPS_OVER_CAN` | *(undefined)* | **Dangerous** — allows CAN temperature injection for debug |
