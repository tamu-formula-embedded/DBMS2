/*
 *
 * Distributed BMS      PWM Interface
 *
 * Copyright (C) 2025   Texas A&M University
 *
 *                      Justus Languell <justus@tamu.edu>
 */

#ifndef _FAN_H_
#define _FAN_H_

#include "context.h"
#include "settings.h"
#include "utils/common.h"

// Q: Wtf is this used for?
// A: Fan control
int InitFan(DbmsCtx *ctx);
int UpdateFan(DbmsCtx *ctx);

#endif
