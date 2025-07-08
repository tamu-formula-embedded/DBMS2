//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _VINTF_H_
#define _VINTF_H_

#include "common.h"
#include "context.h"

#define CANID_TX_HEARTBEAT          0x501
#define CANID_CONSOLE_C0            0x502   // No compression 
#define CANID_CONSOLE_C3            0x505   // Aggressive compression -- Huffman encoding
#define CANID_CELLSTATE_VOLTS       0x507
#define CANID_CELLSTATE_TEMPS       0x508
#define CANID_FATAL_ERROR           0x50B   // = SOB = Son Of a Bitch

#define CANID_RX_HEARTBEAT          0x541
#define CANID_RX_CONFIG             0x542

typedef enum _CanRxChannel {
    CAN_RX_0,
    CAN_RX_1
} CanRxChannel;

// Configure the CAN peripheral
int ConfigCan(HwCtx* hw_ctx);

int CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8]);

int CanReportFault(HwCtx* hw_ctx, char* fn, int line_num, int err_code);

#define CAN_REPORT_FAULT(HW, ERR) CanReportFault(HW, __FILE__, __LINE__, ERR);
// #define CHECK_ERROR_AND_RETURN(STATUS, CALL)     if ((STATUS = CALL) != 0) { CanReportFault(__FILE__, __LINE__, ) }

#define CAN_LOG_BUFFER_SIZE     256     // max formatted string length

void CanLog(HwCtx* hw_ctx, const char* fmt, ...);

void DumpCellState(DbmsCtx* ctx, HwCtx* hw);

int CanTxHeartbeat(HwCtx* hw, uint16_t settings_crc);

int HandleCanConfig(HwCtx* hw, DbmsCtx* ctx, uint8_t* rx_data);

#endif