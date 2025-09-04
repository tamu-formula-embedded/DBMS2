#ifndef _CHARGING_H_
#define _CHARGING_H_

#include "common.h"
#include "context.h"

// to determine if we need balancing
#define BALANCE_LIMIT 1000.0f

// so that we aren't just doing v > min(v) to filter groups - small margin
#define BALANCE_MARGIN 5.0f

bool StackNeedsBalancing(DbmsCtx* ctx);

void StackUpdateBalancing(DbmsCtx* ctx);

bool StackBalancingComplete(DbmsCtx* ctx);

bool StackBalancingAbortedByFault(DbmsCtx* ctx);

void StackBalancingConfig(DbmsCtx* ctx);

#endif
