/*
 * 
 * Distributed BMS      PWM Interface 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */

#ifndef _PWM_H_
#define _PWM_H_

#include "utils/common.h"
#include "context.h"

// TODO: Wtf is this used for?
int ConfigPwmLines(DbmsCtx* ctx);
int SetPwmStates(DbmsCtx* ctx);


#endif
