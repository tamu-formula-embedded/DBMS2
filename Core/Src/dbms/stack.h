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

#define STACK_V_REG_START       0x0568 + 2 * (16 - N_GROUPS_PER_SIDE)
#define STACK_T_REG_START       0x058E // GPIO start reg

#define PACKED __attribute__((packed))

#define ESWAP16(x) __builtin_bswap16(*((uint16_t*)(x)))
#define ESWAP32(x) __builtin_bswap32((uint32_t)(x))

#define SINGLE_DEV_READ         0x80
#define SINGLE_DEV_WRITE        0x90
#define STACK_READ              0xA0  
#define STACK_WRITE             0xB0 
#define BROADCAST_READ          0xC0  
#define BROADCAST_WRITE         0xD0   
#define BROADCAST_WRITE_REV     0xE0   

#define PACKED __attribute__((packed))

#define DATASIZE(...) (sizeof((uint8_t[]) {__VA_ARGS__}) - 1)
#define BYTES16(x) (x) >> 8, (x) & 0xFF

#define MAKE_SINGLE_R(DEV, REG, LEN) {SINGLE_DEV_READ, DEV, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_SINGLE_W(DEV, REG, ...) {SINGLE_DEV_WRITE + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

#define MAKE_STACK_R(REG, LEN) {STACK_READ, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_STACK_W(REG, ...) {STACK_WRITE + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

#define MAKE_BROADCAST_R(REG, LEN) {BROADCAST_READ, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_BROADCAST_W(REG, ...) {BROADCAST_WRITE + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}
#define MAKE_BROADCAST_WR(REG, ...) {BROADCAST_WRITE_REV + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

#define CALC_CRC(F)     CalcCrc16((uint8_t*)(F), ((F)->len + 6 + 1))

typedef struct PACKED _RxStackFrameVoltages
{
    uint8_t     __cmd   : 1;                /* must be set to 0 */
    uint8_t     len     : 7;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[N_GROUPS_PER_SIDE * sizeof(uint16_t)];
    uint16_t    __crc;
} RxStackFrameVoltages;

typedef struct PACKED _RxStackFrameTemps
{
    uint8_t     __cmd   : 1;                /* must be set to 0 */
    uint8_t     len     : 7;                
    uint8_t     devaddr;
    uint16_t    regaddr;
    uint8_t     data[(N_TEMPS_POLL_PER_MONITOR + 2) * sizeof(uint16_t)]; // +2 GPIO  mismatch
    uint16_t    __crc;
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