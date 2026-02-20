/** 
 * 
 * Distributed BMS      Precharge Control 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _PRECHARGE_H_
#define _PRECHARGE_H_

#include "utils/common.h"
#include "context.h"
#include "settings.h"

void PrechargeSet(DbmsCtx *ctx, bool high);
void PrechargeUpdate(DbmsCtx *ctx);

#endif