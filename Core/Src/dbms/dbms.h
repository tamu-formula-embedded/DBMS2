#ifndef _DBMS_H_
#define _DBMS_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "context.h"

#include "current_meter.h"
#include "led_controller.h"
#include "stack_controller.h"
#include "vehicle_interface.h"

// Called before the main loop
void DbmsInit(DbmsCtx* ctx, HwCtx* hw);

// Called from the main loop
void DbmsIter(DbmsCtx* ctx, HwCtx* hw);

// Called to handle an err state
void DbmsErr(DbmsCtx* ctx, HwCtx* hw);

// Called on close
void DbmsClose(DbmsCtx* ctx, HwCtx* hw);

#endif