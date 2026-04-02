# Utils

**Source:** `Core/Src/dbms/utils/`

## Overview

The utils directory contains four small utility modules used across the firmware: CRC16 computation, endian conversion, lookup table interpolation, and a moving average filter.

---

## CRC16 (`crc.c/h`)

`CalcCrc16()` implements the CRC16-Modbus variant using a 256-entry precomputed lookup table for speed. It initializes the shift register to `0xFFFF` and processes each byte via the table. This CRC is used in three places: UART frame validation for BQ79616 stack communication, EEPROM stored-object integrity, and the settings checksum broadcast in the heartbeat.

---

## Endian Swap (`eswap.c/h`)

All external CAN data (IVT-S, Elcon, heartbeat timestamps) arrives big-endian, while the STM32 is little-endian. The eswap module provides inline conversion functions for 16, 32, 48, and 64-bit big-endian values: `be16_to_u16`, `be32_to_u32`, `be48_to_u64`, `be64_to_u64` and their signed variants. The 48-bit functions are specifically for the IVT-S sensor, which uses a 48-bit signed integer representation. Sign extension is handled explicitly: if bit 47 is set, all upper bits are forced to 1.

There are also bulk memory-copy variants `memcpy_eswap2()` and `memcpy_eswap4()` that swap byte order within every 2 or 4 bytes while copying a buffer. `memcpy_eswap2` is used by `SendCellDataBuffer()` to byte-swap 16-bit voltage/temperature values for CAN transmission.

---

## Lookup Table (`lut.c/h`)

The LUT module provides a general-purpose key-value lookup table with linear interpolation. `lut_build()` takes parallel arrays of keys and values, copies them into an array of `LTE` structs, and sorts by key using `qsort`. `lut_interpolate()` performs a binary search to find the bracketing entries and returns the linearly interpolated value.

This is used for thermistor voltage→temperature conversion. The thermistor LUT is built once at startup in `DataInit()` and stored in `ctx->data.lut_therm_v_to_t`. The input-sorted LUT with binary search gives O(log n) lookup time, important since temperature conversion happens for every thermistor reading every iteration.

Note that the ECM battery model (OCV and RC resistance LUTs) does not use this module — those LUTs use a custom indexed lookup with bilinear temperature interpolation implemented directly in `model.c`, since they require a different access pattern (index-based rather than value-based).

---

## Moving Average (`ma.c/h`)

`ma_t` is a fixed-capacity circular buffer moving average for `int32_t` values. `ma_init()` sets up the buffer, `ma_calc_i32()` updates it with a new sample and returns the current average. The average is maintained incrementally via a running sum: the oldest sample is subtracted, the new sample is added, and the sum is divided by the current fill level.

This is used in `CurrentSensor.ima` for smoothing the IVT-S current measurement. However, as of the current code, the `ima` calculation in `faultcheck.c` is commented out, so the moving average is computed into `current_mavg_ma` but the smoothed value is not used for fault decisions — only the raw `current_ma` is checked.

---

## `common.h`

Defines project-wide macros: `MIN`, `MAX`, `CLAMP`, `ARRAY_LEN`, and the standard header includes (`stdint.h`, `stdbool.h`, `string.h`, etc.) that every module needs. Also forward-declares the `DbmsCtx`, `DbmsSettings`, `UserSettingIndex`, and `LedState` types so that headers can reference these types without a full include of `context.h`.
