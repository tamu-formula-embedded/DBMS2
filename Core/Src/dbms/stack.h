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

#define INIT_SNG_R      0x80
#define INIT_SNG_W      0x90
#define INIT_STK_R      0xA0
#define INIT_STK_W      0xB0
#define INIT_BRC_R      0xC0
#define INIT_BRC_W      0xD0
#define INIT_BRC_WR     0xE0

#define PACKED __attribute__((packed))

// typedef struct PACKED _SingleWrite {
//     uint8_t init;
//     uint8_t dev;
//     uint16_t reg;
//     uint8_t data[];
// } SingleWrite;

#define eswap16(X) __builtin_bswap16(X)

#define DATASIZE(...) (sizeof((uint8_t[]) {__VA_ARGS__}) - 1)
#define BYTES16(x) (x) >> 8, (x) & 0xFF

#define MAKE_SNG_R(DEV, REG, LEN) {INIT_SNG_R, DEV, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_SNG_W(DEV, REG, ...) {INIT_SNG_W + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

#define MAKE_STK_R(REG, LEN) {INIT_STK_R, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_STK_W(REG, ...) {INIT_STK_W + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

#define MAKE_BRC_R(REG, LEN) {INIT_BRC_R, BYTES16(REG), LEN, 0x00, 0x00}
#define MAKE_BRC_W(REG, ...) {INIT_BRC_W + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}
#define MAKE_BRC_WR(REG, ...) {INIT_BRC_WR + DATASIZE(__VA_ARGS__), BYTES16(REG), __VA_ARGS__, 0x00, 0x00}

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