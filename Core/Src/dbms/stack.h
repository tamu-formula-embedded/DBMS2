/** 
 * 
 * Distributed BMS      Stack Controller Module
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _STACK_H_
#define _STACK_H_

#include "utils/common.h"

#include "bridge.h"
#include "faults/faults.h"
#include "context.h"
#include "model/data.h"

#include "can.h"
#include "vinterface.h"

#define STACK_V_UV_PER_BIT      190.73
#define STACK_T_UV_PER_BIT      152.59

#define STACK_V_REG_START       (0x0568 + 2 * (16 - N_GROUPS_PER_SIDE))
#define STACK_T_REG_START       0x058E // GPIO start reg

#define PACKED __attribute__((packed))

#define ESWAP16(x) __builtin_bswap16(((uint16_t)(x)))
#define ESWAP32(x) __builtin_bswap32((uint32_t)(x))  

#define REQ_SINGLE_DEV_READ         0   // 0x08
#define REQ_SINGLE_DEV_WRITE        1   // 0x09
#define REQ_STACK_READ              2   // 0x0A
#define REQ_STACK_WRITE             3   // 0x0B
#define REQ_BROADCAST_READ          4   // 0x0C
#define REQ_BROADCAST_WRITE         5   // 0x0D
#define REQ_BROADCAST_WRITE_REV     6   // 0x0E

#define MAX_TX_DATA 8

#define SINGLE_REV_READ(CTX, DEV, REG, ...) SendStackFrame1Dev(CTX, MAKE_TX_FRAME_1DEV(REQ_SINGLE_DEV_READ, DEV, REG, __VA_ARGS__))
#define SINGLE_DEV_WRITE(CTX, DEV, REG, ...) SendStackFrame1Dev(CTX, MAKE_TX_FRAME_1DEV(REQ_SINGLE_DEV_WRITE, DEV, REG, __VA_ARGS__))

#define STACK_READ(CTX, REG, ...) SendStackFrameNDev(CTX, MAKE_TX_FRAME_NDEV(REQ_STACK_READ, REG, __VA_ARGS__))
#define STACK_WRITE(CTX, REG, ...) SendStackFrameNDev(CTX, MAKE_TX_FRAME_NDEV(REQ_STACK_WRITE, REG, __VA_ARGS__))

#define BROADCAST_READ(CTX, REG, ...) SendStackFrameNDev(CTX, MAKE_TX_FRAME_NDEV(REQ_BROADCAST_READ, REG, __VA_ARGS__))
#define BROADCAST_WRITE(CTX, REG, ...) SendStackFrameNDev(CTX, MAKE_TX_FRAME_NDEV(REQ_BROADCAST_WRITE, REG, __VA_ARGS__))
#define BROADCAST_REV_WRITE(CTX, REG, ...) SendStackFrameNDev(CTX, MAKE_TX_FRAME_NDEV(REQ_BROADCAST_WRITE_REV, REG, __VA_ARGS__))

#define PACKED __attribute__((packed))

#define CALC_CRC_Rx(F)     CalcCrc16((uint8_t*)(&F), ((F).len + 6 + 1))
#define FRAME_LEN_SD(F)    ((((F).cmd << 5) >> 5) + 6 + 1)
#define FRAME_LEN_STK(F)    ((((F).cmd << 5) >> 5) + 5 + 1)

typedef struct PACKED _TxStackFrame1Dev 
{
    // uint8_t     __cmd   : 1;                /* must be set to 1 */
    // uint8_t     reqtype : 3;                /* one of REQ_* */
    // uint8_t     __res   : 1;                /* reserved bit */
    // uint8_t     len     : 3;   
    uint8_t cmd;             
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[MAX_TX_DATA];
    uint16_t    __crc;                      /* extra buffer for CRC if all data bytes
                                                are used. So the real location of CRC
                                                is at f->data + f->len */
} TxStackFrame1Dev;

typedef struct PACKED _TxStackFrameNDev 
{
    // uint8_t     __cmd   : 1;                /* must be set to 1 */
    // uint8_t     reqtype : 3;                /* one of REQ_* */
    // uint8_t     __res   : 1;                /* reserved bit */
    // uint8_t     len     : 3;       
    uint8_t cmd;         
    uint16_t    regaddr;
    uint8_t     data[MAX_TX_DATA];
    uint16_t    __crc;                      /* extra buffer for CRC if all data bytes
                                                are used. So the real location of CRC
                                                is at f->data + f->len */
} TxStackFrameNDev;

#define MAKE_TX_FRAME_1DEV(REQTYPE, DEVADDR, REGADDR, ...)   \
((TxStackFrame1Dev){                                         \
    .cmd = (1 << 7) + (REQTYPE << 4) + (sizeof((uint8_t[]){ __VA_ARGS__ })-1), \
    .devaddr = (DEVADDR),                                   \
    .regaddr = ESWAP16(REGADDR),                            \
    .data    = { __VA_ARGS__ },                             \
    .__crc   = 0                                            \
})

#define MAKE_TX_FRAME_NDEV(REQTYPE, REGADDR, ...)          \
((TxStackFrameNDev){                                       \
    .cmd = (1 << 7) + (REQTYPE << 4) + (sizeof((uint8_t[]){ __VA_ARGS__ })-1), \
    .regaddr = ESWAP16(REGADDR),                            \
    .data    = { __VA_ARGS__ },                             \
    .__crc   = 0                                            \
})

#define DATA(...) __VA_ARGS__

typedef struct PACKED _RxStackFrameVoltages
{
    uint8_t     __cmd   : 1;                /* must be set to 0 */
    uint8_t     len     : 7;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[N_GROUPS_PER_SIDE * sizeof(uint16_t)];
    uint16_t    crc;
} RxStackFrameVoltages;

typedef struct PACKED _RxStackFrameTemps
{
    uint8_t     __cmd   : 1;                /* must be set to 0 */
    uint8_t     len     : 7;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[(N_TEMPS_POLL_PER_MONITOR + 2) * sizeof(uint16_t)]; // +2 GPIO  mismatch
    uint16_t    crc;
} RxStackFrameTemps;

/**
 * @brief Wake the battery stack
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int StackWake(DbmsCtx* ctx);

/**
 * @brief Full auto addressing procedure
 * 
 * @param ctx Context pointer 
 */
void StackAutoAddr(DbmsCtx* ctx);

/**
 * @brief Set the number of active cells in the stack
 * 
 * @param ctx Context pointer 
 * @param n_active_cells 
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells);

/**
 * Config heartbeat timeout
 */
void StackConfigTimeout(DbmsCtx* ctx);

/**
 * @brief Shuts down the battery stack
 * Use this when we turn the vehicle off, or when going to sleep
 * We could also use this in response to a critical fault
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int StackShutdown(DbmsCtx* ctx);


/*****************************
 *  VOLTAGE/TEMP READINGS 
 *****************************/

/**
 * @brief Enable the voltage tap ADCs
 * 
 * @param ctx Context pointer 
 */
void StackSetupVoltReadings(DbmsCtx* ctx); 

/**
 * @brief Request and populate voltage readings for the whole stack
 * 
 * @param ctx Context pointer 
 */
void StackUpdateAllVoltReadings(DbmsCtx* ctx);

/**
 * @brief Configure GPIOs for temp readings
 * 
 * @param ctx Context pointer 
 */
void StackSetupGpio(DbmsCtx* ctx);

void StackUpdateAllTempReadings(DbmsCtx* ctx);

void UpdateTemps(DbmsCtx* ctx, RxStackFrameTemps* frame);

void UpdateVoltages(DbmsCtx* ctx, RxStackFrameVoltages* frame);
int StackRead(DbmsCtx* ctx, uint8_t* raw, uint16_t start_reg, uint8_t data_size, int expected_size);

/**
 * @brief Replace missing thermistor readings with the average of the valid cells
 * 
 * @param ctx Context pointer 
 */
void FillMissingTempReadings(DbmsCtx* ctx);

/**
 * @brief Compute min, avg, max voltages and temperatures 
 * 
 * @param ctx Context pointer 
 */
void StackCalcStats(DbmsCtx* ctx);

void IncStackCrcStats(DbmsCtx* ctx, bool good, int monitor_id);

/*****************************
 *   LED CONTROL
 *****************************/

// TODO: document + reeval confusing names

int ToggleMonitorLeds(DbmsCtx* ctx, bool on);

void MonitorLedBlink(DbmsCtx* ctx);

/*****************************
 *  BALANCING 
 *****************************/

typedef enum {
    BAL_TIME_0S = 0,
    BAL_TIME_10S,
    BAL_TIME_30S,
    BAL_TIME_60S,
    __N_BAL_TIMES
} StackBalanceTimes;

void StackBalancingConfig(DbmsCtx* ctx);

void StackStartBalancing(DbmsCtx* ctx, bool odds, int32_t bal_time);

void StackSetDeviceBalanceTimers(DbmsCtx* ctx, uint8_t dev_addr, bool odds, 
        StackBalanceTimes bal_time_idx);

void StackComputeCellsToBalance(DbmsCtx* ctx, bool odds, int32_t threshold_mv);

bool StackNeedsToBalance(DbmsCtx* ctx, bool odds, int32_t threshold_mv);

void StackDumpCellsToBalance(DbmsCtx* ctx);

int SetMuxChannels(DbmsCtx* ctx, uint8_t channel);

int ReadMuxOutputs4x1(DbmsCtx* ctx, uint8_t dev_number, float* gpio3, float* gpio4, float* gpio5, float* gpio6);
#endif