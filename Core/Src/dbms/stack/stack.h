/** 
 * 
 * Distributed BMS      Stack Module
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#ifndef _STACK_H_
#define _STACK_H_

#include "../utils/common.h"

#include "../faults/faults.h"
#include "../context.h"
#include "../model/data.h"

#include "../can.h"
#include "../vinterface.h"

#define STACK_V_UV_PER_BIT 190.73
#define STACK_T_UV_PER_BIT 152.59

// TODO: optimize the shit out of this
#define STACK_SEND_TIMEOUT 50
#define STACK_RECV_TIMEOUT 10

#define APBxCLK 42000000    // TODO: fix legacy name

#define RX_FRAME_SIZE(SIZE) (SIZE+6)

/*****************************
 *   STACK I/O
 *****************************/

/**
 * Sends a raw data frame to the battery stack via UART
 */
int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len);

/**
 * Sends a data frame to the stack via UART with
 * a CRC appended to the end for verification
 */
int SendStackFrameSetCrc(DbmsCtx* ctx, uint8_t* buf, size_t len);

/**
 * Utility function to set the baud rate of the UART
 * Used for shutdown and wakeup
 */
void SetBrr(uint64_t brr);

/**
 * Send a "blip", where we pull the line low for a little bit.
 * 
 * Right now the length of the blip is set by BRR.
 *
 * TODO: make it accept us or something more intuitive.
 */
int SendStackBlip(DbmsCtx* ctx, uint64_t brr);

/*****************************
 *   CONTROL FUNCTIONS
 *****************************/

/**
 * Wake the battery stack
 */
int StackWake(DbmsCtx* ctx);

/**
 * Full auto addressing procedure
 */
void StackAutoAddr(DbmsCtx* ctx);


/**
 * Set the number of active cells in the stack
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells);

/**
 * Shuts down the battery stack
 * Use this when we turn the vehicle off, or when going to sleep
 * We could also use this in response to a critical fault
 */
int StackShutdown(DbmsCtx* ctx);


/*****************************
 *  VOLT READINGS 
 *****************************/

/**
 * Enable the voltage tap ADCs
 */
void StackSetupVoltReadings(DbmsCtx* ctx); 


/**
 * Request and populate voltage readings for a monitor by side-addr
 */
void StackUpdateVoltReadingSingle(DbmsCtx* ctx, uint16_t addr);


/*****************************
 *  TEMP READINGS 
 *****************************/

/**
 * Configure GPIOs for temp readings
 */
void StackSetupGpio(DbmsCtx* ctx);


/**
 * Read temperatures from a monitor by side-addr and if/if not it is the sidekick monitor
 * 
 * TODO: more explaination 
 */
void StackUpdateTempReadingSingle(DbmsCtx* ctx, uint16_t addr, bool sidekick);

/**
 * Replace missing thermistor readings with the average of the valid cells
 */
void FillMissingTempReadings(DbmsCtx* ctx);

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

 void StackBalancingConfig(DbmsCtx* ctx);

void StackSetDeviceBalanceTimer(DbmsCtx* ctx, uint8_t device_addr, bool cells_to_balance[N_GROUPS_PER_SIDE]);

void StackStartDeviceBalancing(DbmsCtx* ctx, uint8_t device_addr);

void StackStartBalancing(DbmsCtx* ctx);

bool StackIsDoneBalancing(DbmsCtx* ctx);


#endif