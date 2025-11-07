#include "charging.h"
#include "context.h"
#include "ledctl.h"
#include "stack.h"

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

bool NeedsToBalance(DbmsCtx* ctx)
{
    // TODO: condition for BMS temperature < some threshold
    return ctx->stats.max_v - ctx->stats.min_v > GetSetting(ctx, CH_BAL_DELTA) &&
           ctx->stats.min_v > GetSetting(ctx, CH_BAL_MIN_V);
}

bool ChargingComplete(DbmsCtx* ctx)
{
    return ctx->stats.min_v > GetSetting(ctx, CH_TARGET_V);
}


static uint32_t bal_times[] = { 0, 10, 30, 60 };
#define LOOKUP_BAL_TIMES(T) (bal_times[MIN((uint8_t)T, (uint8_t)__N_BAL_TIMES - 1)])
#define TIME_IN_STATE_MS(CTX_PTR) (HAL_GetTick() - CTX_PTR->charging.state_enter_ts)

bool DoneBalancing(DbmsCtx* ctx)
{
    return TIME_IN_STATE_MS(ctx) > LOOKUP_BAL_TIMES(GetSetting(ctx, CH_BAL_T_IDX)); // TODO: impl condition based on status
}


void ChargingEnterState(DbmsCtx* ctx, ChargingState new_state)
{
    if (ctx->charging.state == new_state) return;

    ctx->charging.prev_state = ctx->charging.state;
    ctx->charging.state = new_state;
    ctx->charging.state_enter_ts = HAL_GetTick();

    switch (new_state)
    {
    case CH_NO_CONN:
        break;
    case CH_CHARGING:
        break;

    case CH_BALANCING_ODDS:
        StackComputeCellsToBalance(ctx, GetSetting(ctx, CH_BAL_DELTA));
        // Sends balance timers and starts charging:
        StackStartBalancing(ctx, true, GetSetting(ctx, CH_BAL_T_IDX));
        break;

    case CH_BALANCING_EVENS:
        StackComputeCellsToBalance(ctx, GetSetting(ctx, CH_BAL_DELTA));
        StackStartBalancing(ctx, false, GetSetting(ctx, CH_BAL_T_IDX));
        break;

    case CH_COMPLETE:
        break;
    }
}

void ChargingUpdate(DbmsCtx* ctx)
{
    J1772ReadState(ctx);

    ctx->charging.conn = ChargingConnected(ctx);
    // if (charge_conn && !ctx->charging.conn)         ;       // can register an action here
    // else if (!charge_conn && ctx->charging.conn)    ;       // can register an action here
    // ctx->charging.conn = charge_conn;

    HAL_Delay(1);

    CanLog(ctx, "CH = %d\tTH = %d\n", ctx->charging.state, GetSetting(ctx, CH_BAL_DELTA));

    // Seperate condition to detect no connection because it is a
    // global transition that would need to be implemented in every state
    if (!ctx->charging.conn)
    {
        ChargingEnterState(ctx, CH_NO_CONN);
    }

    switch (ctx->charging.state) // Main state switch
    {
    case CH_NO_CONN:
        SendElconRequest(ctx, 0, 0, 0);

        if (ctx->charging.conn) ChargingEnterState(ctx, CH_CHARGING);

        break;

    case CH_CHARGING:
        ctx->led_state = LED_CHARGING;
        SendElconRequest(ctx, 0, 0, 0); // TODO: figure out what to put here

        if (NeedsToBalance(ctx)) ChargingEnterState(ctx, CH_BALANCING_ODDS);

        if (ChargingComplete(ctx)) ChargingEnterState(ctx, CH_COMPLETE);

        break;

    case CH_BALANCING_ODDS:
        ctx->led_state = LED_BALANCING;
        SendElconRequest(ctx, 0, 0, 0);

        if (DoneBalancing(ctx)) ChargingEnterState(ctx, CH_BALANCING_EVENS);

        break;

    case CH_BALANCING_EVENS:
        ctx->led_state = LED_BALANCING;
        SendElconRequest(ctx, 0, 0, 0);

        if (DoneBalancing(ctx)) ChargingEnterState(ctx, CH_BALANCING_EVENS);

        break;

    case CH_COMPLETE:
        ctx->led_state = LED_CHARGING;  // TODO: new LED state
        SendElconRequest(ctx, 0, 0, 0);

        break;
    }
}
