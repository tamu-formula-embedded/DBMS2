/** 
 * 
 * Distributed BMS      J1772 Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _J1772_H_
#define _J1772_H_

#include "context.h"
#include "storage.h"
#include "vinterface.h"

// TODO: make this a setting
void J1772SetPPGoodThresholdMv(DbmsCtx* ctx, int threshold);

void J1772SetPWMReadTimeout(DbmsCtx* ctx, int num);

void J1772SetChargeEnable(DbmsCtx* ctx, bool en);

void J1772ReadState(DbmsCtx* ctx);

#endif