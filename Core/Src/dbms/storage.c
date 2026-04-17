/** 
 * 
 * Distributed BMS      EEPROM Storage Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#include "storage.h"

extern I2C_HandleTypeDef hi2c1;

int WriteEEPROM(DbmsCtx* ctx, uint32_t addr, uint8_t* data, uint16_t len)
{
    int status = 0;
    uint16_t remaining = len, offset = 0;

    while (remaining > 0)
    {
        uint16_t page_offset = addr % EEPROM_PAGE_SIZE;
        uint16_t write_size = EEPROM_PAGE_SIZE - page_offset;
        if (write_size > remaining) write_size = remaining;

        uint8_t buf[2 + EEPROM_PAGE_SIZE];
        buf[0] = (addr >> 8) & 0xFF; // Address high byte (A15-A8)
        buf[1] = addr & 0xFF;        // Address low byte (A7-A0)
        memcpy(&buf[2], &data[offset], write_size);

        uint8_t dev_addr = EEPROM_ADDR | ((addr >> 16) & 0x01); // A16 bit
        if ((status = HAL_I2C_Master_Transmit(ctx->hw.i2c, dev_addr, buf, write_size + 2, EEPROM_WRITE_TIMEOUT)) !=
            HAL_OK)
        {
            return status;
        }

        HAL_Delay(5);
        addr += write_size;
        offset += write_size;
        remaining -= write_size;
    }

    ctx->stats.n_eeprom_writes++;

    return HAL_OK;
}

int ReadEEPROM(DbmsCtx* ctx, uint32_t addr, uint8_t* data, uint16_t len)
{
    int status = 0;

    uint8_t addr_buf[2];
    addr_buf[0] = (addr >> 8) & 0xFF;
    addr_buf[1] = addr & 0xFF;
    uint8_t dev_addr = EEPROM_ADDR | ((addr >> 16) & 0x01);

    if ((status = HAL_I2C_Master_Transmit(ctx->hw.i2c, dev_addr, addr_buf, 2, EEPROM_WRITE_TIMEOUT)) != HAL_OK)
    {
        return status;
    }

    return HAL_I2C_Master_Receive(ctx->hw.i2c, dev_addr, data, len, EEPROM_READ_TIMEOUT);
}

int SaveStoredObject(DbmsCtx* ctx, uint32_t mem_addr, void* object, size_t obj_size)
{
    uint8_t buf[obj_size + sizeof(uint16_t)];
    memcpy(buf, object, obj_size);

    uint16_t crc = CalcCrc16((uint8_t*)object, obj_size);
    memcpy(buf + obj_size, &crc, sizeof(crc));

    return WriteEEPROM(ctx, mem_addr, buf, sizeof(buf));
}

int LoadStoredObject(DbmsCtx* ctx, uint32_t mem_addr, void* object, size_t obj_size)
{
    int status = 0;
    uint8_t buf[obj_size + sizeof(uint16_t)];

    if ((status = ReadEEPROM(ctx, mem_addr, buf, sizeof(buf))) != HAL_OK)
    {
        return status;
    }

    memcpy(object, buf, obj_size);

    uint16_t r_crc;
    memcpy(&r_crc, buf + obj_size, sizeof(r_crc));

    uint16_t c_crc = CalcCrc16((uint8_t*)object, obj_size);
    if (r_crc != c_crc)
    {
        status = ERR_CRC_MISMATCH;
    }

    return status;
}