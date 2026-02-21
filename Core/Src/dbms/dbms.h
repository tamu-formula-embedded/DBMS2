/** 
 * 
 * Distributed BMS      DBMS Main Controller
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _DBMS_H_
#define _DBMS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/common.h"
#include "context.h"
#include "stack.h"
#include "model/data.h"
#include "model/model.h"
#include "faults/faults.h"
#include "ledctl.h"
#include "sched.h"
#include "settings.h"
#include "storage.h"
#include "can.h"
#include "vinterface.h"
#include "fan.h"
#include "isense.h"
#include "elcon.h"
#include "charging.h"
#include "precharge.h"


// Called before the main loop but strictly used for memory allocation
void DbmsAlloc(DbmsCtx* ctx);

// Called before the main loop
void DbmsInit(DbmsCtx* ctx);

// Called from the main loop
void DbmsIter(DbmsCtx* ctx);

// Called to handle an err state
void DbmsErr(DbmsCtx* ctx);

// Called on close
void DbmsClose(DbmsCtx* ctx);

// Called on CAN RX
void DbmsCanRx(DbmsCtx* ctx, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8]);

#endif