#include "charging.h"
#include "context.h"
#include "ledctl.h"
#include "stack/stack.h"

bool ElconConnected(DbmsCtx* ctx)
{
    return HAL_GetTick() - ctx->elcon.heartbeat < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);
}

bool ChargingAllowed(DbmsCtx* ctx)
{
    return HAL_GetTick() - ctx->charging.heartbeat < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);
}

bool ChargingConnected(DbmsCtx* ctx)
{
    return ctx->active && /*ctx->charging.allowed &&*/ ElconConnected(ctx) /*&& ctx->j1772.pp_connect*/;
}

void ChargingEnter(DbmsCtx* ctx)
{
    ctx->charging.state = CH_CHARGING;
}

void ChargingUpdate(DbmsCtx* ctx)
{   
    J1772ReadState(ctx);

    if (ctx->charging.state == CH_CONNECTING)
    {
        SendElconRequest(ctx, 0, 0, 0);
    }

    bool charge_conn = ChargingConnected(ctx);
    if (charge_conn && !ctx->charging.conn)
        ChargingEnter(ctx);
    else if (!charge_conn && ctx->charging.conn)
        ChargingExit(ctx);
    ctx->charging.conn = charge_conn;

    HAL_Delay(1);

    CanLog(ctx, "CH = %d\tTH = %d\n", ctx->charging.state, GetSetting(ctx, CELL_BALANCE_LIMIT));

    // if (ctx->active)
    //     StackReadBalStat(ctx, 0);

    // HAL_Delay(2);

    // if (ctx->active && HAL_GetTick() - ctx->wakeup_ts > 10000 && ctx->charging.state != CH_BALANCING && StackNeedsBalancing(ctx))
    // {
    //     ctx->charging.state = CH_BALANCING;   
    //     CanLog(ctx, "Balancing:\n");
    //     ctx->led_state = LED_BALANCING;
    //     StackDumpCellsToBalance(ctx);
    //     HAL_Delay(2);
    //     StackStartBalancing(ctx);
    //     HAL_Delay(10);
    // }
    // else
    // {
    //     ctx->charging.state = CH_CHARGING;
    //     ctx->led_state = LED_CHARGING;
    // }
    // HAL_Delay(10);
}

void ChargingExit(DbmsCtx* ctx)
{
    ctx->charging.state = CH_CONNECTING;
}

