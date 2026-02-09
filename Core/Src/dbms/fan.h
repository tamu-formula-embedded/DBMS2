/** 
 * 
 * Distributed BMS      Fan Controller
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _FAN_H_
#define _FAN_H_

#include "context.h"
#include "settings.h"
#include "utils/common.h"
#include "vinterface.h"

// Q: Wtf is this used for?
// A: Fan control
int InitFan(DbmsCtx *ctx);
int UpdateFan(DbmsCtx *ctx);

#endif
