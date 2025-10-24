/** 
 * 
 * Distributed BMS      CAN Current Sensor
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 */
#include "can.h"

void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time)
{
    static uint8_t frame_set_stop_mode[8] = {0x34, 0, 1, 0, 0, 0, 0, 0};
    static uint8_t frame_set_run_mode[8] = {0x34, 1, 1, 0, 0, 0, 0, 0};
    static uint8_t frame_set_metric_cycle[8] = {0x20, 2, 0, 0, 0, 0, 0, 0};
    frame_set_metric_cycle[2] = (cycle_time & 0xFF00) >> 8;
    frame_set_metric_cycle[3] = (cycle_time & 0x00FF) >> 0;

    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_stop_mode);
    HAL_Delay(1);

    frame_set_metric_cycle[0] = 0x21;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x22;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x25;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x26;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x27;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);

    HAL_Delay(1);
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_run_mode);
    HAL_Delay(1);
}

int64_t UnpackCurrentSensorData(uint8_t* data) // expects >= 6 bytes, big-endian
{
    // Assemble into the low 48 bits
    uint64_t v = 0;
    v |= ((uint64_t)data[0] << 40);
    v |= ((uint64_t)data[1] << 32);
    v |= ((uint64_t)data[2] << 24);
    v |= ((uint64_t)data[3] << 16);
    v |= ((uint64_t)data[4] << 8);
    v |= ((uint64_t)data[5] << 0);

    // Sign bit is bit 47 (counting from 0). If set, extend the sign.
    if (v & (1ULL << 47))
    {
        v |= ~((1ULL << 48) - 1); // set all upper bits to 1
    }
    return (int64_t)v;
}
