#include "charging.h"

void ChargingEnter(DbmsCtx* ctx)
{
    // set to balancing for now
    ctx->charging.state = CH_BALANCING;
}

void ChargingUpdate(DbmsCtx* ctx)
{
    

}

void ChargingExit(DbmsCtx* ctx)
{

}

