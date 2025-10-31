#include "charging.h"
#include "context.h"

void ChargingEnter(DbmsCtx* ctx)
{
    // set to balancing for now
    ctx->charging.state = CH_BALANCING;
}

void ChargingUpdate(DbmsCtx* ctx)
{
    if(ctx->charging.state == CH_BALANCING)
    {
        BalancingTest(ctx);
    }
}

void ChargingExit(DbmsCtx* ctx)
{

}

