//
//  Copyright (C) Texas A&M University
//
//  vintf -- Vehicle Interface
//
#ifndef _VINTF_H_
#define _VINTF_H_

#include "common.h"
#include "context.h"

// Configure the CAN peripheral
bool ConfigCan(HwCtx* hw_ctx);

bool CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8]);

#endif