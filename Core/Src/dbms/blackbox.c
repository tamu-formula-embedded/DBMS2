#include "blackbox.h"
#include "context.h"
#include "storage.h"
#include "vehicle_interface.h"
#include "common.h"

void BlackboxInit(DbmsCtx* ctx)
{
    memset(ctx->blackbox.old, 0, sizeof(Snapshot));
    memset(ctx->blackbox.new, 0, sizeof(Snapshot));
}

void BlackboxSwapAndUpdate(DbmsCtx* ctx)
{
    Snapshot* temp = ctx->blackbox.old;
    ctx->blackbox.old = ctx->blackbox.new;
    ctx->blackbox.new = temp;

    PopulateSnapshot(ctx, ctx->blackbox.new);
}

void PopulateSnapshot(DbmsCtx* ctx, Snapshot* snapshot)
{
    // populate the snapshot with all the important 
    // information every loop

    snapshot->iter = ctx->stats.iters;
}
