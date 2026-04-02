# Stack

**Source:** `Core/Src/dbms/stack.c`, `stack.h`

## Overview

The Stack module is the highest-level abstraction over the BQ79616 cell monitor chips. It builds on the raw UART framing provided by [Bridge](bridge.md) to implement the full register-level protocol: startup and auto-addressing, continuous voltage and temperature acquisition, cell balancing control, and the analog MUX cycling for thermistor expansion.

---

## Frame Abstraction

Stack communicates via two frame shapes: single-device frames (`TxStackFrame1Dev`) target a specific device address, and multi-device frames (`TxStackFrameNDev`) are used for stack reads, stack writes, and broadcast operations. The command byte encodes the request type and the payload length in a compact 8-bit field. Convenience macros (`SINGLE_DEV_WRITE`, `BROADCAST_WRITE`, `STACK_READ`, etc.) are used throughout the code to construct and send these frames without boilerplate.

Response frames from the stack carry the device address, register address, data bytes, and a CRC16 in a packed struct (`RxStackFrameVoltages`, `RxStackFrameTemps`). All data on the TI COM bus is big-endian and uses 190.73 µV/bit for voltage and 152.59 µV/bit for GPIO ADC temperature readings.

---

## Startup Sequence

`DbmsPerformWakeup()` calls the following stack functions in order, with small delays between each step:

**Wake:** `StackWake()` sends the UART baud-rate "wake blip" followed by a `CTRL1_SEND_WAKE` broadcast write, repeated twice. This ensures all devices on the daisy chain transition from sleep to active.

**Auto-addressing:** `StackAutoAddr()` runs TI's 6-step auto-addressing procedure:
1. Zero out the OTP ECC data-in registers (clears address register shadows)
2. Broadcast `CTRL1_ADDR_WR` to put all devices into address-write mode
3. Send incrementing addresses from 0 to N_STACKDEVS on `REG_DIR0_ADDR` — each device in the chain latches the next value
4. Configure stack topology: all monitor chips set as stack devices via `COMM_STACK_DEV`, the bridge set as non-stack (address 0), and the top-of-stack device additionally marked with `COMM_TOP_STACK`
5. Read back the OTP ECC test registers to verify addressing succeeded

After addressing, `StackSetNumActiveCells()` sets the cell count register (0x0A = 10 decimal, selecting 13 differential measurements from cells 1–16 minus the unused top groups), and `StackSetupVoltReadings()` starts the ADC in continuous-run mode.

---

## Voltage Readings

`StackUpdateAllVoltReadings()` issues a single STACK_READ command starting at `STACK_V_REG_START` (computed from `N_GROUPS_PER_SIDE` to select the correct cell registers) and reads back `N_GROUPS_PER_SIDE × 2` bytes from all `N_MONITORS` devices in one transaction. The response arrives as concatenated per-device frames, which are parsed sequentially.

For each device response, the CRC is verified. If the CRC passes, the 13 big-endian 16-bit values are scaled by 190.73 µV/bit and stored in `ctx->cell_states[side].voltages[j]` in millivolts. A failed CRC increments the per-monitor bad-CRC counter rather than updating voltages — the previous reading is kept.

The device address in the response frame is decoded via `ADDR_BCAST_TO_STACK()` to map from the BQ79600 device addressing scheme to the firmware's 0-indexed `cell_states` array.

---

## Temperature Readings and MUX Cycling

Each BQ79616 has up to 8 GPIO pins that can be used as ADC inputs. By connecting each GPIO to one channel of a 4:1 analog multiplexer, and running 13 thermistors through a combination of multiplexed and direct GPIO connections, each monitor chip can read 13 thermistor voltages — but only a subset per iteration, depending on which MUX channel is selected.

`SetMuxChannels()` writes a specific GPIO configuration byte to register `0x000F` on all stack devices to select one of four MUX channels. This is called at the **end** of each main loop iteration, so the next iteration reads the newly selected channel. Since the MUX cycles through 4 positions across iterations, each thermistor is actually updated once every 4 iterations (~200 ms at 20 Hz).

`StackUpdateAllTempReadings()` reads the GPIO ADC registers, converts the raw ADC values to volts (152.59 µV/bit), and calls `ThermVoltToTemp()` to map voltage to temperature using the calibrated thermistor LUT. The mux offset determines which temperature indices within `cell_states[side].temps[]` are filled on a given iteration.

`UpdateTemps()` handles an important subtlety: the GPIO register layout in the BQ79616 does not sequentially map to GPIO1–GPIO6. A `memmove` is used to reorder the data bytes before parsing, moving the first two GPIO readings into positions 4 and 5 in the buffer (effectively remapping GPIO1→GPIO3 and GPIO2→GPIO4 in the software representation).

**Missing temperature handling:** With `IGNORE_BAD_THERMS` enabled, `FillMissingTempReadings()` runs after each temperature update. Any reading outside the plausibility range (`LOW_PLAUSIBLE_TEMP` to `HIGH_PLAUSIBLE_TEMP`) is replaced with the pack average of all valid readings. This prevents a single broken thermistor from causing unnecessary faults or corrupting average temperature calculations.

---

## Statistics

After each volt and temp read, `StackCalcStats()` computes min, max, and average voltage and temperature across all sides and groups, and sums all group voltages into `stats.pack_v`. These aggregates are used by fault checks, the battery model, and telemetry.

---

## Cell Balancing

The BQ79616 supports bypass-resistor cell balancing. Balance timers are configured per-cell in registers `REG_CB_CELL1_CTRL` through `REG_CB_CELL16_CTRL`. The timer value selects a balancing duration (0 = off, 1–3 correspond to 10/30/60 seconds via the `StackBalanceTimes` enum). Balancing is committed by writing `BAL_GO` to `REG_BAL_CTRL2`, which starts all configured timers simultaneously.

`StackComputeCellsToBalance()` builds a boolean mask of which cells need balancing, based on comparing each cell's pre-balance average voltage to `pre_bal_min_v + threshold`. Only odd-indexed or even-indexed cells are marked in a given pass (controlled by the `odds` argument), enforcing the interleaved balancing strategy described in [Charging](charging.md).

`StackStartBalancing()` iterates over all monitor devices, programs each device's balance timers for the appropriate odd/even subset, then fires `BAL_GO` on each. Auto-balancing mode (`BAL2_AUTO_BAL`) is enabled at startup, and `BAL2_FLTSTOP_EN` causes balancing to automatically pause if a thermal or fault condition is detected by the chip hardware.

---

## Monitor LED

The BQ79616 chips have an LED connected to GPIO8. `MonitorLedBlink()` controls this LED as a visual heartbeat: it blinks 100 ms on, 1000 ms off, toggled via `ToggleMonitorLeds()` which writes to `REG_GPIO_CONF4` on all stack devices. This blink is independent of the control board LEDs and indicates that the firmware is actively polling the monitor chips.

---

## Interactions

Stack is called from [DBMS](dbms.md) for wakeup, per-iteration volt/temp reads, and stats. [Bridge](bridge.md) provides the actual UART send/receive functions. [Charging](charging.md) calls the balancing functions. [Data/Model](model.md) provides `ThermVoltToTemp()` for ADC-to-temperature conversion.
