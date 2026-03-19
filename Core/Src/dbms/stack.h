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

#define STACK_V_UV_PER_BIT 190.73
#define STACK_T_UV_PER_BIT 152.59

#define PACKED __attribute__((packed))

#define ESWAP16(x) __builtin_bswap16((uint16_t)(x))
#define ESWAP32(x) __builtin_bswap32((uint32_t)(x))

#define MAX_TX_DATA 8
#define MAX_RX_DATA 128

#define REQ_SINGLE_DEV_READ         0   // 0x08
#define REQ_SINGLE_DEV_WRITE        1   // 0x09
#define REQ_STACK_READ              2   // 0x0A
#define REQ_STACK_WRITE             3   // 0x0B
#define REQ_BROADCAST_READ          4   // 0x0C
#define REQ_BROADCAST_WRITE         5   // 0x0D
#define REQ_BROADCAST_WRITE_REV     6   // 0x0E

typedef struct PACKED _TxStackFrameDEV 
{
    uint8_t     __cmd   : 1;                /* must be set to 1 */
    uint8_t     reqtype : 3;                /* one of REQ_* */
    uint8_t     __res   : 1;                /* reserved bit */
    uint8_t     len     : 3;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[MAX_TX_DATA];
    uint16_t    __crc;                      /* extra buffer for CRC if all data bytes
                                                are used. So the real location of CRC
                                                is at f->data + f->len */
} TxStackFrameDEV;

#define CRC_VALUE(F)          *((uint16_t*)((F).data + (F).len + 1))

#define CALC_CRC(F)     CalcCrc((uint8_t*)(&F), ((F).reqtype < 2 ? 4 + (F).len + 1 : 3 + (F).len + 1))

#define FRAME_LEN(F)    ((F).len + 6)

#define DATA(...) __VA_ARGS__

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