#ifndef DBMS_DATA_H
#define DBMS_DATA_H

#include "context.h"

#include "lut.h"

#define N_THERM_V_TO_T_ENTRIES      121
#define N_OCV_ENTRIES               201
#define N_RC_ENTRIES                101
#define N_C_ENTRIES                 101

extern const LTE lut_therm_v_to_t[N_THERM_V_TO_T_ENTRIES];
void MakeLutThermistors();

extern const float ocv_t_high[N_OCV_ENTRIES];
extern const float ocv_t_low[N_OCV_ENTRIES];

extern const float rc_t_high[3][N_RC_ENTRIES];
extern const float rc_t_low[3][N_RC_ENTRIES];

#endif
