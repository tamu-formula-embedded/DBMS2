#ifndef _BLACKBOX_H_
#define _BLACKBOX_H_

#include "context.h"
#include "fault_handler.h"


// Initialize blackbox system - allocates memory for old and new blackbox
void BlackboxInit(DbmsCtx* ctx);

void BlackboxSwapAndUpdate(DbmsCtx* ctx);

void PopulateBlackboxInfo(DbmsCtx* ctx, BlackboxInfo* blackbox);

BlackboxInfo* GetBlackboxOld(DbmsCtx* ctx);
BlackboxInfo* GetBlackboxNew(DbmsCtx* ctx);

#endif