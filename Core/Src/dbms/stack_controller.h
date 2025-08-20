//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _STACKCTL_H_
#define _STACKCTL_H_

#include "common.h"
#include "context.h"

#include "sched.h"

#include "vehicle_interface.h"

#define STACK_SEND_TIMEOUT          100
#define STACK_RECV_TIMEOUT          100
#define STACK_RX_BUFFER_SIZE        1024    //N_STACKDEVS * 6 + N_GROUPS

#define RX_FRAME_SIZE(DATA_SIZE)    (DATA_SIZE + 6)

typedef struct {            // ordering packed
    uint8_t* data;
    size_t size;
    uint16_t reg_addr;
    uint16_t crc;
    uint8_t init_field;
    uint8_t dev_addr;
} RxStackFrame;

void __PrintStackRxFrame(RxStackFrame* f);

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
