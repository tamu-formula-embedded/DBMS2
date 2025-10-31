#ifndef _J1772_H_
#define _J1772_H_

#include "context.h"
#include "storage.h"

// TODO: make this a setting?
#define PP_LOW_THRESHOLD_MV     2200
#define PWM_READ_TIMEOUT        500

void J1772SetChargeEnable(DbmsCtx* ctx, bool en);

void J1772ReadState(DbmsCtx* ctx);

#endif