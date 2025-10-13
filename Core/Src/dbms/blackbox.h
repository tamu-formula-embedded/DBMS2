#ifndef _BLACKBOX_H_
#define _BLACKBOX_H_

#include "context.h"
#include "fault_handler.h"

void BlackboxInit(DbmsCtx* ctx);

void BlackboxSwapAndUpdate(DbmsCtx* ctx);

void PopulateBlackboxInfo(DbmsCtx* ctx, Snapshot* blackbox);

int BlackboxSend(DbmsCtx* ctx);

int BlackboxSaveOnFault(DbmsCtx* ctx, Snapshot* old_blackbox, Snapshot* new_blackbox);


#endif