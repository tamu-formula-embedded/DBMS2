#ifndef DBMS_DATA_H
#define DBMS_DATA_H

#include "context.h"

#include "lut.h"


extern const float ocv_t_high[N_OCV_ENTRIES];
extern const float ocv_t_low[N_OCV_ENTRIES];

extern const float rc_t_high[3][N_RC_ENTRIES];
extern const float rc_t_low[3][N_RC_ENTRIES];


void DataInit(DbmsCtx* ctx);

float ThermVoltToTemp(DbmsCtx* ctx, float V);

#endif
