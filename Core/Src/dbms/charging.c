#include "charging.h"
#include "context.h"
#include "ledctl.h"
#include "stack.h"
#include "vinterface.h"

bool ElconConnected(DbmsCtx* ctx)
{
    return (HAL_GetTick() - ctx->elcon.heartbeat < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN))&&!CtrlHasAnyFaults(ctx);
}

bool ChargingAllowed(DbmsCtx* ctx)
{
    return HAL_GetTick() - ctx->charging.heartbeat < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);
}

bool ChargingConnected(DbmsCtx* ctx)
{
    return ctx->active && /*ctx->charging.allowed &&*/ ElconConnected(ctx) && ctx->j1772.pp_connect;
}

bool NeedsToBalance(DbmsCtx* ctx)   // TODO: this is the start condition
{
    // TODO: condition for BMS temperature < some threshold
    // TODO: if we have a cell at max V
    return 1000 * (ctx->stats.max_v - ctx->stats.min_v) > GetSetting(ctx, CH_BAL_DELTA_BEGIN) 
        || (1000 * ctx->stats.max_v > GetSetting(ctx, CH_TARGET_V) 
           && 1000 * (ctx->stats.max_v - ctx->stats.min_v) > GetSetting(ctx, CH_BAL_DELTA_END));
}

bool NeedsToBalanceMore(DbmsCtx* ctx)
{
    return 1000 * (ctx->stats.max_v - ctx->charging.pre_bal_min_v) > GetSetting(ctx, CH_BAL_DELTA_END);
            // && 1000 * ctx->stats.min_v > GetSetting(ctx, CH_BAL_MIN_V);
}

bool ChargingComplete(DbmsCtx* ctx)
{
    return ctx->stats.max_v > GetSetting(ctx, CH_TARGET_V) 
        && 1000 * (ctx->stats.max_v - ctx->stats.min_v) < GetSetting(ctx, CH_BAL_DELTA_END);
}


static uint32_t bal_times[] = { 0, 10, 30, 60 };
#define LOOKUP_BAL_TIMES(T) (bal_times[MIN((uint8_t)T, (uint8_t)__N_BAL_TIMES - 1)])
#define TIME_IN_STATE_MS(CTX_PTR) (HAL_GetTick() - CTX_PTR->charging.state_enter_ts)

bool DoneBalancing(DbmsCtx* ctx)
{
    int32_t bal_t_idx = GetSetting(ctx, CH_BAL_T_IDX);
    int32_t times = LOOKUP_BAL_TIMES(bal_t_idx) * 1000 + 6000;
    CanLog(ctx, "Times %d\n", times);
    return TIME_IN_STATE_MS(ctx) > times; // TODO: impl condition based on status
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
        CanLog(ctx, "Enter No Comms\n");
        for (int i = 0; i < N_SIDES; i++) {
            memset(ctx->cell_states[i].cells_to_balance, 0, sizeof(ctx->cell_states[i].cells_to_balance));
        }
        SendCellsToBalance(ctx);
        break;
    case CH_CHARGING:
        CanLog(ctx, "Enter Charging\n");
        for (int i = 0; i < N_SIDES; i++) {
            memset(ctx->cell_states[i].cells_to_balance, 0, sizeof(ctx->cell_states[i].cells_to_balance));
        }
        SendCellsToBalance(ctx);
        break;

    case CH_BALANCING_ODDS:
        CanLog(ctx, "Enter Bal Odds\n");
        ctx->charging.pre_bal_min_v = ctx->stats.min_v;
        StackComputeCellsToBalance(ctx, true, GetSetting(ctx, CH_BAL_DELTA_END)); 
        SendCellsToBalance(ctx);
        // Sends balance timers and starts charging:
        StackStartBalancing(ctx, true, GetSetting(ctx, CH_BAL_T_IDX));
        break;

    case CH_BALANCING_EVENS:
        CanLog(ctx, "Enter Bal Evens\n");
        StackComputeCellsToBalance(ctx, false, GetSetting(ctx, CH_BAL_DELTA_END));
        SendCellsToBalance(ctx);
        StackStartBalancing(ctx, false, GetSetting(ctx, CH_BAL_T_IDX));
        break;

    case CH_COMPLETE:
        CanLog(ctx, "Enter Complete\n");
        for (int i = 0; i < N_SIDES; i++) {
            memset(ctx->cell_states[i].cells_to_balance, 0, sizeof(ctx->cell_states[i].cells_to_balance));
        }
        SendCellsToBalance(ctx);
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

    // CanLog(ctx, "CH = %d\tTH = %d\n", ctx->charging.state, GetSetting(ctx, CH_BAL_DELTA));

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

        int32_t v_req = MIN(GetSetting(ctx, CH_TARGET_V) * N_GROUPS_PER_SIDE * N_SIDES, 600000) / 1000;
        int32_t i_req = MIN(MIN(GetSetting(ctx, CH_I), ctx->j1772.maxCurrentSupply), 25);
        //SendElconRequest(ctx, v_req, i_req, 1);
        CanLog(ctx, "Elcon V=%d I=%d\n", ctx->elcon.v_req, ctx->elcon.i_req);

        if (TIME_IN_STATE_MS(ctx) > 1000)   // TODO:?
        {
            if (NeedsToBalance(ctx)) ChargingEnterState(ctx, CH_BALANCING_ODDS);

            if (ChargingComplete(ctx)) ChargingEnterState(ctx, CH_COMPLETE);
        }

        break;

    case CH_BALANCING_ODDS:
        ctx->led_state = LED_BALANCING_ODDS;
        SendElconRequest(ctx, 0, 0, 0);

        if (DoneBalancing(ctx)) ChargingEnterState(ctx, CH_BALANCING_EVENS);

        break;

    case CH_BALANCING_EVENS:
        ctx->led_state = LED_BALANCING_EVENS;
        SendElconRequest(ctx, 0, 0, 0);

        if (DoneBalancing(ctx))
        {
            if (NeedsToBalanceMore(ctx))
                ChargingEnterState(ctx, CH_BALANCING_ODDS);
            else
                ChargingEnterState(ctx, CH_CHARGING);
        }
            

        break;

    case CH_COMPLETE:
        ctx->led_state = LED_CHARGING_COMPLETE;  // TODO: new LED state
        SendElconRequest(ctx, 0, 0, 0);

        break;
    }
}
