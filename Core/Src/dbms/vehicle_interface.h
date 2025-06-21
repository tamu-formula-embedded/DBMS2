//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _VINTF_H_
#define _VINTF_H_

#include "common.h"
#include "context.h"

// Configure the CAN peripheral
bool ConfigCan(HwCtx* hw_ctx);

bool CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8]);

#define CAN_LOG_BUFFER_SIZE     256     // max formatted string length
#define CAN_LOG_ID              0x5F2  

void CanLog(HwCtx* hw_ctx, const char* fmt, ...);

#endif