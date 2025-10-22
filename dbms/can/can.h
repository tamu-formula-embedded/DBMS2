/** 
 * 
 * Distributed BMS      CAN Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#ifndef _CAN_H_
#define _CAN_H_

#include "../utils/common.h"
#include "../context.h"

#include "../ledctl.h"
#include "../sched.h"
#include "../settings.h"

#define CAN_STD_ID_MASK         0x7FF
#define CAN_EXT_ID_MASK         0x1FFFFFFF
#define CAN_LOG_BUFFER_SIZE     512             // max formatted string length
#define CAN_TX_WAIT_US          200             // polling sleep step
#define CAN_TX_TIMEOUT_US       3000

#define ERR_CFGID_NOT_FOUND     54


#define CANID_ISENSE_COMMAND        0x411

#define CANID_TX_HEARTBEAT          0x501
#define CANID_CONSOLE_C0            0x502 // No compression
#define CANID_CONSOLE_C3            0x505 // Aggressive compression -- Huffman encoding
#define CANID_METRIC                0x506
#define CANID_CELLSTATE_VOLTS       0x507
#define CANID_CELLSTATE_TEMPS       0x508
#define CANID_FATAL_ERROR           0x50B // = SOB = Son Of a Bitch

#define CANID_ISENSE_CURRENT        0x521
#define CANID_ISENSE_VOLTAGE1       0x522
#define CANID_ISENSE_VOLTAGE2       0x523
#define CANID_ISENSE_VOLTAGE3       0x524

#define CANID_ISENSE_POWER          0x526
#define CANID_ISENSE_CHARGE         0x527
#define CANID_ISENSE_ENERGY         0x528

#define CANID_BLACKBOX_OLD          0x529
#define CANID_BLACKBOX_NEW          0x530

#define CANID_TX_GET_CONFIG         0x533
#define CANID_TX_CFG_ACK            0x532

#define CANID_RX_TELEMBEAT          0x540
#define CANID_RX_HEARTBEAT          0x541
#define CANID_RX_SET_CONFIG         0x542
#define CANID_RX_GET_CONFIG         0x543
#define CANID_RX_CLEAR_FAULTS       0x544
#define CANID_RX_SET_INITIAL_CHARGE 0x545
#define CANID_RX_BLACKBOX_REQUEST   0x546
#define CANID_RX_BLACKBOX_SAVE      0x547

#define CANID_DEBUG_OVERWRITE_TEMPS 0x581

#define CANID_ELCON_A               0x1806E5F4
#define CANID_ELCON_B               0x18FF50E5



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
 * Send plex metrics                        (version 1)
 */
int SendPlexMetrics(DbmsCtx* ctx);

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
