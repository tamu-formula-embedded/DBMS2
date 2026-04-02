# Storage

**Source:** `Core/Src/dbms/storage.c`, `storage.h`

## Overview

The storage module is the I2C driver for the 24CSM01 EEPROM, providing page-aligned write, sequential read, and a higher-level `SaveStoredObject`/`LoadStoredObject` pair that automatically appends and verifies a CRC16 checksum.

---

## EEPROM Memory Map

| Address | Content |
|---------|---------|
| `0x0000` (page 0) | Settings (`DbmsSettings` struct + CRC16) |
| `0x0100` (page 1) | Fault mask (`controller_mask` + CRC16) |
| `0x0120` (page 1 + 32) | Charge stats (`ChargeStats` struct + CRC16) |
| `0x0200` (page 2) | Blackbox metadata (head, count) |
| `0x0300` (page 3+) | Blackbox snapshots |
| `0x0400` (page 4) | Debug counter (`n_int_shutdowns`) |

The 24CSM01 has 128-byte pages. `WriteEEPROM()` automatically splits writes that cross page boundaries into multiple I2C transactions, inserting a 5 ms delay between transactions to respect the EEPROM's write cycle time.

---

## Stored Object Pattern

`SaveStoredObject()` serializes an arbitrary struct to EEPROM by appending a CRC16 of the data bytes immediately after the struct in the write buffer. `LoadStoredObject()` reads the same number of bytes plus 2, re-computes the CRC, and returns `ERR_CRC_MISMATCH` if they don't match. The caller (settings, faults, charge stats) decides how to handle a mismatch — typically by loading defaults and scheduling a re-write.

This pattern is used for all persistent state in the system. It means any struct layout change will automatically be detected as a CRC mismatch on the next boot, triggering a fallback to defaults. There is no versioning scheme beyond the CRC.

---

## Interactions

`SaveStoredObject` and `LoadStoredObject` are called by [Settings](settings.md), [Faults](faults.md), [Model](model.md) (charge stats), and [Blackbox](blackbox.md). The raw `ReadEEPROM`/`WriteEEPROM` functions are also called directly for the debug counter and blackbox metadata.
