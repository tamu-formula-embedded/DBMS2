#ifndef _MODEL_H_
#define _MODEL_H_

#include "context.h"
#include "data.h"
#include "lut.h"
#include "storage.h"

#define Q_BOUND_L_OC    4.3696    
#define Q_BOUND_H_OC    4.3967
#define Q_BOUND_L_RC    4.3914    
#define Q_BOUND_H_RC    4.197    
#define T_H_PT          45
#define T_L_PT          25

float ThermVoltToTemp(float V);

void UpdateModel(DbmsCtx* ctx);

int LoadInitialCharge(DbmsCtx* ctx);
int SaveInitialCharge(DbmsCtx* ctx);

#endif