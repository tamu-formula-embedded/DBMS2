//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _VINTF_H_
#define _VINTF_H_

#include "common.h"
#include "context.h"

typedef enum _CanRxChannel {
    CAN_RX_0,
    CAN_RX_1
} CanRxChannel;

// Configure the CAN peripheral
int ConfigCan(HwCtx* hw_ctx);

int CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8]);

#define CAN_LOG_BUFFER_SIZE     256     // max formatted string length
#define CAN_LOG_ID              0x5F2  

void CanLog(HwCtx* hw_ctx, const char* fmt, ...);

void DumpCellState(DbmsCtx* ctx, HwCtx* hw);

#endif