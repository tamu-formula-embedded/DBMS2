/** 
 * 
 * Distributed BMS      Charging State-Machine Controller
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _CHARGING_H_
#define _CHARGING_H_

#include "context.h"
#include "storage.h"
#include "elcon.h"
#include "j1772.h"

// void ChargingEnter(DbmsCtx* ctx);

void ChargingUpdate(DbmsCtx* ctx);

// void ChargingExit(DbmsCtx* ctx);

#endif