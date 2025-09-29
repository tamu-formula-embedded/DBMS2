#include "blackbox.h"
#include "context.h"
#include "storage.h"
#include "vehicle_interface.h"
#include "common.h"

void BlackboxInit(DbmsCtx* ctx)
{
    memset(ctx->blackbox_old, 0, sizeof(BlackboxInfo));
    memset(ctx->blackbox_new, 0, sizeof(BlackboxInfo));
}

void BlackboxSwapAndUpdate(DbmsCtx* ctx)
{
    // swap the pointers
    BlackboxInfo* temp = ctx->blackbox_old;
    ctx->blackbox_old = ctx->blackbox_new;
    ctx->blackbox_new = temp;
    
    // populate the new blackbox with current state
    PopulateBlackboxInfo(ctx, ctx->blackbox_new);
}

void PopulateBlackboxInfo(DbmsCtx* ctx, BlackboxInfo* blackbox)
{
    // this happens every iteration - update all important info
    // blackbox->important = ctx->important
    blackbox->iter = ctx->stats.iters;
}

// getters
BlackboxInfo* GetBlackboxOld(DbmsCtx* ctx)
{
    return ctx->blackbox_old;
}

BlackboxInfo* GetBlackboxNew(DbmsCtx* ctx)
{
    return ctx->blackbox_new;
}
