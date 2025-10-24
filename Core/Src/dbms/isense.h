/** 
 * 
 * Distributed BMS      CAN Current Sensor
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 */
#ifndef _ISENSE_H_
#define _ISENSE_H_

#include "can.h"

#include "utils/common.h"
#include "context.h"

#include "ledctl.h"
#include "sched.h"
#include "settings.h"

void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time);

int64_t UnpackCurrentSensorData(uint8_t* data); // expects >= 6 bytes, big-endian


#endif