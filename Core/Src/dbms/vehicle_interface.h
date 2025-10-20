//
//  Copyright (c) Texas A&M University.
//
#ifndef _VINTF_H_
#define _VINTF_H_

#include "utils/common.h"
#include "context.h"

#include "ledctl.h"
#include "sched.h"
#include "settings.h"

#define CANID_ISENSE_COMMAND 0x411

#define CANID_TX_HEARTBEAT    0x501
#define CANID_CONSOLE_C0      0x502 // No compression
#define CANID_CONSOLE_C3      0x505 // Aggressive compression -- Huffman encoding
#define CANID_METRIC          0x506
#define CANID_CELLSTATE_VOLTS 0x507
#define CANID_CELLSTATE_TEMPS 0x508
#define CANID_FATAL_ERROR     0x50B // = SOB = Son Of a Bitch

#define CANID_ISENSE_CURRENT  0x521
#define CANID_ISENSE_VOLTAGE1 0x522
#define CANID_ISENSE_VOLTAGE2 0x523
#define CANID_ISENSE_VOLTAGE3 0x524

#define CANID_ISENSE_POWER  0x526
#define CANID_ISENSE_CHARGE 0x527
#define CANID_ISENSE_ENERGY 0x528

#define CANID_BLACKBOX_OLD  0x529
#define CANID_BLACKBOX_NEW  0x530

#define CANID_TX_GET_CONFIG 0x533
#define CANID_TX_CFG_ACK    0x532

#define CANID_RX_TELEMBEAT          0x540
#define CANID_RX_HEARTBEAT          0x541
#define CANID_RX_SET_CONFIG         0x542
#define CANID_RX_GET_CONFIG         0x543
#define CANID_RX_CLEAR_FAULTS       0x544
#define CANID_RX_SET_INITIAL_CHARGE 0x545
#define CANID_RX_BLACKBOX_REQUEST   0x546
#define CANID_RX_BLACKBOX_SAVE      0x547

#define CANID_DEBUG_OVERWRITE_TEMPS 0x581

#define ERR_CFGID_NOT_FOUND 54

#define CAN_TX_WAIT_US    200 // polling sleep step
#define CAN_TX_TIMEOUT_US 3000

typedef enum _CanRxChannel
{
    CAN_RX_0,
    CAN_RX_1
} CanRxChannel;

typedef enum _CanConfigAction
{
    CFG_SET,
    CFG_GET
} CanConfigAction;

// Configure the CAN peripheral
int ConfigCan(DbmsCtx* ctx);

int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8]);

int CanReportFault(DbmsCtx* ctx, char* fn, int line_num, int err_code);

#define CAN_REPORT_FAULT(CTX, ERR) CanReportFault(CTX, __FILE__, __LINE__, ERR);
// #define CHECK_ERROR_AND_RETURN(STATUS, CALL)     if ((STATUS = CALL) != 0) { CanReportFault(__FILE__, __LINE__, ) }

#define CAN_LOG_BUFFER_SIZE 512 // max formatted string length

void CanLog(DbmsCtx* ctx, const char* fmt, ...);

int SendCellVoltages(DbmsCtx* ctx);
int SendCellTemps(DbmsCtx* ctx);
int SendMetrics(DbmsCtx* ctx);

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc);

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action);

void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time);
int64_t UnpackCurrentSensorData(uint8_t* data);

int ConfigPwmLines(DbmsCtx* ctx);
int SetPwmStates(DbmsCtx* ctx);

#endif