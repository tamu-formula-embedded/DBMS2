#ifndef _BLACKBOX_H_
#define _BLACKBOX_H_

#include "context.h"
#include "fault_handler.h"

void BlackboxInit(DbmsCtx* ctx);

void BlackboxSaveOnFault(DbmsCtx* ctx);

void BlackboxSwapAndUpdate(DbmsCtx* ctx);

void PopulateBlackboxInfo(DbmsCtx* ctx, Snapshot* blackbox);

int BlackboxSend(DbmsCtx* ctx);

int SaveBlackboxToEEPROM(DbmsCtx* ctx, Snapshot* old_blackbox, Snapshot* new_blackbox);

Snapshot* GetBlackboxOld(DbmsCtx* ctx);
Snapshot* GetBlackboxNew(DbmsCtx* ctx);
Snapshot* GetBlackboxSavedOld(DbmsCtx* ctx);
Snapshot* GetBlackboxSavedNew(DbmsCtx* ctx);

#endif