//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _STACKCTL_H_
#define _STACKCTL_H_

#include "common.h"
#include "context.h"

#include "vehicle_interface.h"

#define STACK_SEND_TIMEOUT      100
#define STACK_RECV_TIMEOUT      100
#define STACK_RX_BUFFER_SIZE    256

typedef struct {
    uint8_t init_field;
    uint8_t dev_addr;
    uint16_t reg_addr;
    uint8_t buffer[STACK_RX_BUFFER_SIZE];
    uint8_t* data;
    size_t size;
} RxStackFrame;

#define STACK_DEFINE_RX_FRAME(NAME, S)\
    RxStackFrame NAME = {.init_field = 0, .dev_addr = 0, .reg_addr = 0, .buffer = {0}, .data = NULL, .size = S};\
    NAME.data = NAME.buffer + 4;

void __PrintStackRxFrame(RxStackFrame* f);

void DelayUs(DbmsCtx* ctx, uint16_t us);

#define APBxCLK 42000000    // TODO: fix legacy name

void SetBrr(uint64_t brr);

uint16_t CalcCrc16(uint8_t *buf, size_t len);

int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len);

int SendStackFrameSetCrc(DbmsCtx* ctx, uint8_t* buf, size_t len);

int StackWake(DbmsCtx* ctx);

int StackShutdown(DbmsCtx* ctx);

void StackAutoAddr(DbmsCtx* ctx);

void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells);

void StackSetupGpio(DbmsCtx* ctx);

void StackSetupVoltReadings(DbmsCtx* ctx);
void StackUpdateVoltReadings(DbmsCtx* ctx);

#endif
