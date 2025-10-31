#include "charging.h"
#include "context.h"
#include "ledctl.h"
#include "stack/stack.h"

void ChargingEnter(DbmsCtx* ctx)
{
    ctx->charging.state = CH_CHARGING;
    ctx->cur_state = DBMS_CHARGING;
}

void ChargingUpdate(DbmsCtx* ctx)
{

    if(ctx->charging.state == CH_CHARGING && StackNeedsBalancing(ctx))
    {     
        ctx->charging.state = CH_BALANCING;
        ctx->led_state = LED_BALANCING;
        //StackDumpCellsToBalance(ctx);
        //StackUpdateBalancing(ctx);
    }

    if(ctx->charging.state == CH_BALANCING)
    {
        // todo: monitor balancing state
    }
}

void ChargingExit(DbmsCtx* ctx)
{

}

