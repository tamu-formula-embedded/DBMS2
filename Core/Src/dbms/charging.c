#include "charging.h"

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
    return ctx->active && ctx->charging.allowed && ElconConnected(ctx) && ctx->j1772.pp_connect;
}

void ChargingEnter(DbmsCtx* ctx)
{
    CanLog(ctx, "Entering charging state\n");
}

void ChargingUpdate(DbmsCtx* ctx)
{
    J1772ReadState(ctx);

    ctx->charging.allowed = HAL_GetTick() - ctx->charging.heartbeat < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);
    if (ctx->charging.allowed)
    {
        SendElconRequest(ctx, 0, 0, 0);
    }

    bool charge_conn = ChargingConnected(ctx);
    if (charge_conn && !ctx->charging.conn)
        ChargingEnter(ctx);
    else if (!charge_conn && ctx->charging.conn)
        ChargingExit(ctx);
    ctx->charging.conn = charge_conn;

    CanLog(ctx, "Yo bal\n");
}

void ChargingExit(DbmsCtx* ctx)
{
    CanLog(ctx, "Exiting charging state\n");
}

