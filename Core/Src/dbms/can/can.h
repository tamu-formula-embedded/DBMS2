/*
 * 
 * Distributed BMS      CAN Interface 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */

#ifndef _CAN_H_
#define _CAN_H_

#include "../utils/common.h"
#include "../context.h"

#include "../ledctl.h"
#include "../sched.h"
#include "../settings.h"

#include "canids.h"

#define CAN_STD_ID_MASK         0x7FF
#define CAN_EXT_ID_MASK         0x1FFFFFFF
#define CAN_LOG_BUFFER_SIZE     512             // max formatted string length
#define CAN_TX_WAIT_US          200             // polling sleep step
#define CAN_TX_TIMEOUT_US       3000

#define ERR_CFGID_NOT_FOUND     54

typedef enum 
{
    CAN_RX_0, 
    CAN_RX_1,
} CanRxChannel;

typedef enum 
{
    CFG_SET,
    CFG_GET
} CanConfigAction;

int ConfigCan(DbmsCtx* ctx);

int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8]);

/*****************************
 *   TELEMETRY
 *****************************/

/**
 * Send cell voltage readings array         (version 1)
 */
int SendCellVoltages(DbmsCtx* ctx);

/**
 * Send cell voltage readings array         (version 1)
 */
int SendCellTemps(DbmsCtx* ctx);

/**
 * Send individual metrics                  (version 1)
 */
int SendMetrics(DbmsCtx* ctx);

/**
 * Send a formatted log message to the app  (version 1)
 */
void CanLog(DbmsCtx* ctx, const char* fmt, ...);

/**
 * Deprecated
 */
#define CAN_REPORT_FAULT(CTX, ERR)  do {} while (0)

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc);

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action);

/*****************************
 *   PWM LINE FOR ????
 *****************************/

// TODO: Wtf is this used for?
int ConfigPwmLines(DbmsCtx* ctx);
int SetPwmStates(DbmsCtx* ctx);


/*****************************
 *   CURRENT SENSOR
 *****************************/

/**
 * Start the current sensor and configure the data channels
 */
void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time);

/**
 * Unpack a data reading from the current sensor
 */
int64_t UnpackCurrentSensorData(uint8_t* data);


/**
 * TODO: move these functions out of here
 * TODO: fix the impl to use [ ]
 */
int32_t UnpackElconDataVoltage(uint8_t* data);
int32_t UnpackElconDataCurrent(uint8_t* data);


#endif
