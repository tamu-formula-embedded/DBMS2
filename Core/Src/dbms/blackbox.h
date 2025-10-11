#ifndef _BLACKBOX_H_
#define _BLACKBOX_H_

#include "context.h"
#include "fault_handler.h"


// Initialize blackbox system - allocates memory for old and new blackbox
void BlackboxInit(DbmsCtx* ctx);

void BlackboxSwapAndUpdate(DbmsCtx* ctx);

void PopulateSnapshot(DbmsCtx* ctx, Snapshot* blackbox);

Snapshot* GetBlackboxOld(DbmsCtx* ctx);
Snapshot* GetBlackboxNew(DbmsCtx* ctx);

#endif