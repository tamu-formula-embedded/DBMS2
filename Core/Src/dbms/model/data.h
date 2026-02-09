/** 
 * 
 * Distributed BMS      Static Data for LUT's etc
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef DBMS_DATA_H
#define DBMS_DATA_H

#include "../context.h"

#include "../utils/lut.h"


extern const float ocv_t_high[N_OCV_ENTRIES];
extern const float ocv_t_low[N_OCV_ENTRIES];

extern const float rc_t_high[3][N_RC_ENTRIES];
extern const float rc_t_low[3][N_RC_ENTRIES];


void DataInit(DbmsCtx* ctx);

float ThermVoltToTemp(DbmsCtx* ctx, float V);

#endif
