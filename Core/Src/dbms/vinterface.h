/** 
 * 
 * Distributed BMS      Vehicle Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#ifndef _VINTERFACE_H_
#define _VINTERFACE_H_

#include "can.h"

#include "utils/common.h"
#include "context.h"

#include "ledctl.h"
#include "sched.h"
#include "settings.h"


/*****************************
 *   TELEMETRY
 *****************************/


int SendCellVoltages(DbmsCtx* ctx);

int SendCellTemps(DbmsCtx* ctx);

int SendMetrics(DbmsCtx* ctx);

void SendPlexMetrics(DbmsCtx* ctx);

/**
 * Send a formatted log message to the app  
 */
void CanLog(DbmsCtx* ctx, const char* fmt, ...);

/**
 * Deprecated
 */
#define CAN_REPORT_FAULT(CTX, ERR)  do {} while (0)

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc);

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action);


#endif