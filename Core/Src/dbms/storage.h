//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "common.h"
#include "context.h"

#include "vehicle_interface.h"

#define EEPROM_WRITE_TIMEOUT    100     // todo: drop
#define EEPROM_READ_TIMEOUT     100     // 

#define EEPROM_ADDR             (0x50 << 1) 
#define EEPROM_PAGE_SIZE        256
#define EEPROM_SETTINGS_ADDR    0x0000

#define ERR_CRC_MISMATCH        24

int WriteEEPROM(DbmsCtx* ctx, uint32_t addr, uint8_t* data, uint16_t len);
int ReadEEPROM(DbmsCtx* ctx, uint32_t addr, uint8_t* data, uint16_t len);

int SaveSettings(DbmsCtx* ctx, DbmsSettings* settings);
int LoadSettings(DbmsCtx* ctx, DbmsSettings* settings);

#endif