# LedCtl

**Source:** `Core/Src/dbms/ledctl.c`, `ledctl.h`

## Overview

The LED controller maps a single `LedState` enum value (set by the DBMS each iteration) to physical outputs on three dual-color (red/green) LEDs. The design keeps LED logic fully decoupled from the rest of the firmware — any module simply writes `ctx->led_state` and the LED controller handles the rest.

---

## Hardware

There are three LEDs, each with independent red and green channels:
- LED 0: Red on PC6, Green on PC7
- LED 1: Red on PC8, Green on PC9
- LED 2: Red on PA8, Green on PA9

Red and green can be driven independently, giving three output colors: red, green, and yellow (both on). The `LedColor` enum encodes these as a 2-bit value where bit 0 = red and bit 1 = green, making `LedSet()` a direct bitmask write to two GPIO pins.

---

## State Table

Each system state maps to a specific color and blink type for all three LEDs simultaneously. The mapping is defined as a static compile-time table in `ledctl.c`:

| State | LED 0 | LED 1 | LED 2 |
|-------|-------|-------|-------|
| `LED_FIRMWARE_FAULT` | Red solid | Red solid | Red solid |
| `LED_IDLE` | Yellow fast | Yellow solid | Yellow solid |
| `LED_IDLE_FAULT` | Red fast | Yellow solid | Yellow solid |
| `LED_INIT` | Yellow solid | Green solid | Green solid |
| `LED_ACTIVE` | Green fast | Green solid | Green solid |
| `LED_ACTIVE_FAULT` | Red fast | Green solid | Green solid |
| `LED_CHARGING` | Green fast | Green fast | Green fast |
| `LED_CHARGING_WAIT` | Yellow fast | Yellow fast | Yellow fast |
| `LED_BALANCING_EVENS` | Yellow fast | Yellow solid | Yellow fast |
| `LED_BALANCING_ODDS` | Yellow fast | Yellow fast | Yellow solid |
| `LED_CHARGING_COMPLETE` | Green fast | Green fast | Green solid |

The asymmetry in `LED_BALANCING_EVENS` vs. `LED_BALANCING_ODDS` (LEDs 1 and 2 swapped) makes it visually distinguishable which balancing pass is active.

---

## Blink Implementation

`ProcessLedAction()` runs every main loop iteration. It determines whether any LED in the current state pattern needs blinking, and if so, whether the blink period has elapsed — if it has, the global `blink_on` toggle flips. All LEDs in the current pattern that have a blink type share the same blink toggle, so they stay synchronized. Fast blink is 100 ms interval, slow blink is 500 ms.

There is also `MonitorLedBlink()`, which controls an LED physically on the BQ79616 monitor chips via GPIO8. This blinks independently at a 100 ms on / 1 second off pattern as a "heartbeat" indicator that the firmware is communicating with the stack.

---

## Interactions

Any module can change the visible LED state by writing `ctx->led_state`. The [DBMS](dbms.md) main loop sets it based on active/fault/shutdown conditions; [Charging](charging.md) overrides it during charging phases. `ProcessLedAction()` is called at the end of every `DbmsIter()` iteration.
