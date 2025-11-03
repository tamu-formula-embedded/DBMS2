/**
 * Distributed BMS Stack Module
 * 
 * @file stack.h
 * @copyright (C) 2025 Texas A&M University
 * @author Justus Languell <justus@tamu.edu>
 *         Cam Stone        <cameron28202@tamu.edu>
 *         Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 * 
 * @brief Exposes interface for controlling 
 * the stack through the bridge module.
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
 * @brief Request and populate voltage readings for a monitor by side-addr
 * 
 * @param ctx Context pointer 
 * @param addr Index of the side to read for
 */
void StackUpdateVoltReadingSingle(DbmsCtx* ctx, uint16_t addr);

/**
 * @brief Configure GPIOs for temp readings
 * 
 * @param ctx Context pointer 
 */
void StackSetupGpio(DbmsCtx* ctx);

/**
 * @brief Read temperatures from a monitor by side-addr and if/if not it is the sidekick monitor
 * 
 * @todo More explaination
 * 
 * @param ctx Context pointer 
 * @param addr Index of the side to read for
 * @param sidekick Read the main chip or the sidekick chip
 */
void StackUpdateTempReadingSingle(DbmsCtx* ctx, uint16_t addr, bool sidekick);

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

int ToggleMonitorChipLed(DbmsCtx* ctx, bool on, uint8_t dev_number);

int ToggleAllMonitorChipLeds(DbmsCtx* ctx, bool on);

void MonitorLedBlink(DbmsCtx* ctx);

/*****************************
 *   FAULT READING
 *****************************/

// TODO: clean this one up when we look into this stuff more
//       there all very legacy

int SetFaultMasks(DbmsCtx* ctx);

int PollFaultSummary(DbmsCtx* ctx);

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

void StackComputeCellsToBalance(DbmsCtx* ctx, int32_t threshold_mv);

void StackDumpCellsToBalance(DbmsCtx* ctx);

/**
 * @brief Request and populate voltage readings for a monitor by side-addr.
 * 
 * @param ctx Context pointer
 * @param addr Index of side to poll 
 */
void StackReadBalStat(DbmsCtx* ctx, uint16_t addr);


#endif