//
//  Copyright (c) Texas A&M University.
//
#ifndef _STACKCTL_H_
#define _STACKCTL_H_

#include "common.h"
#include "context.h"

#include "sched.h"

#include "vehicle_interface.h"

#include "data.h"
#include "model.h"

#define STACK_SEND_TIMEOUT 100
#define STACK_RECV_TIMEOUT 100
#define STACK_RX_BUFFER_SIZE 1024 // N_STACKDEVS * 6 + N_GROUPS_PER_SIDE

#define STACK_V_UV_PER_BIT 190.73
#define STACK_T_UV_PER_BIT 152.59

#define STACK_FAULT_REG_N   27

#define STACK_DEV_CONF_REG  0x0002

#define BRIDGE_DEV_CONF1    0x2001

#define BRIDGE_FAULT_COMM1_REG  0x2104

#define FAULT_SUMM_BITMASK 0b11101100   // DO NOT CHANGE WITHOUT TALKING 
                                        // WITH THE TEAM FIRST PLSSS!!

#define RX_FRAME_SIZE(DATA_SIZE) (DATA_SIZE + 6)

typedef struct
{ // ordering packed
    uint8_t* data;
    size_t size;
    uint16_t reg_addr;
    uint16_t crc;
    uint8_t init_field;
    uint8_t dev_addr;
} RxStackFrame;

void __PrintStackRxFrame(RxStackFrame* f);

#define APBxCLK 42000000 // TODO: fix legacy name

void SetBrr(uint64_t brr);

uint16_t CalcCrc16(uint8_t* buf, size_t len);

int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len);

int SendStackFrameSetCrc(DbmsCtx* ctx, uint8_t* buf, size_t len);

int StackWake(DbmsCtx* ctx);

int StackShutdown(DbmsCtx* ctx);

void StackAutoAddr(DbmsCtx* ctx);

void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells);

void StackSetupGpio(DbmsCtx* ctx);
void StackUpdateTempReadings(DbmsCtx* ctx);

void StackSetupVoltReadings(DbmsCtx* ctx);
void StackUpdateVoltReadings(DbmsCtx* ctx);

int ToggleAllMonitorChipLeds(DbmsCtx* ctx, bool on);

void FillStackFrame(RxStackFrame* rx_frame, uint8_t* buffer, size_t size);

void FillStackFrames(RxStackFrame* rx_frames, uint8_t* buffer, size_t size, size_t n_frames);

void MonitorLedBlink(DbmsCtx* ctx);

void FillMissingTempReadings(DbmsCtx* ctx);

int Bridge_Dev_Conf_FAULT_EN(DbmsCtx* ctx);
int Stack_Dev_Conf_FAULT_EN(DbmsCtx* ctx);
// void Parse_FAULT_COMM1(DbmsCtx* ctx, uint8_t data);

int Read_Bridge_Fault_Comm(DbmsCtx* ctx);
int SetFaultMasks(DbmsCtx* ctx);
int PollFaultSummary(DbmsCtx* ctx);
#endif
