//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _STACKCTL_H_
#define _STACKCTL_H_

#include "common.h"
#include "context.h"

#define STACKCTL_SEND_TIMEOUT 1

void DelayUs(HwCtx* hw, uint16_t us);

void SetBrr(uint64_t brr);

uint16_t CalcCrc16(uint8_t *buf, size_t len);

void SendStackFrame(HwCtx* hw, uint8_t* buf, size_t len);

void SendStackFrameSetCrc(HwCtx* hw, uint8_t* buf, size_t len);

void SendStackWake(HwCtx* hw);

void SendStackShutdown(HwCtx* hw);

void StackAutoAddr(HwCtx* hw);

#endif