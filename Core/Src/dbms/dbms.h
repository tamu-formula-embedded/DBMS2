//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _DBMS_H_
#define _DBMS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "context.h"

#include "sched.h"
#include "settings.h"
#include "vehicle_interface.h"  // should go first?
#include "current_meter.h"
#include "led_controller.h"
#include "stack_controller.h"
#include "storage.h"
#include "fault_handler.h"
#include "charging.h"



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