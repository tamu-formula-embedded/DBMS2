//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"

//
//  Allocate space for pointer objects
//
void DbmsAlloc(DbmsCtx* ctx)
{
    static DbmsSettings mem_settings;
    ctx->settings = &mem_settings;

    static PerfCounters mem_perf_counters;
    ctx->perfctrs = &mem_perf_counters;
}

//
//  Initialization loop
//
void DbmsInit(DbmsCtx* ctx)
{
    int status = 0;
    ctx->cur_state = DBMS_SHUTDOWN;
    ctx->led_state = LED_INIT;
    if ((status = LoadSettings(ctx)) != HAL_OK)
    {
        CAN_REPORT_FAULT(ctx, status);
        if (status == ERR_CRC_MISMATCH)
        {
            // This is a bad situation, but we can still proceed
            // by loading fallback settings? Ideally these should
            // be updated enough to be ok
            LoadFallbackSettings(ctx);
            ctx->need_to_sync_settings = true;          // queue up a write
        }  
        else    {}  // fatal error
}
    //memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(ctx->hw.timer);

    if ((status = ConfigCan(ctx) != HAL_OK))
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return;
    }
    // set to idle or active? i think idle because we would want to call dbmsperformwakeup?
    ctx->led_state = LED_IDLE;
}

int DbmsPerformWakeup(DbmsCtx* ctx)
{
    int status = 0;
    HAL_Delay(2000);

    if ((status = StackWake(ctx)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return status;                  // we are cooked
    }
    ctx->cur_state = DBMS_ACTIVE;
    ctx->led_state = LED_ACTIVE;

    StackAutoAddr(ctx);
    StackSetNumActiveCells(ctx, 0x0A);
    StackSetupGpio(ctx);
    StackSetupVoltReadings(ctx);     // todo: rn start

    return status;
}

int DbmsPerformShutdown(DbmsCtx* ctx)
{
    int status = 0;
    if ((status = StackShutdown(ctx)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return status;
    }
    ctx->cur_state = DBMS_SHUTDOWN;
    ctx->led_state = LED_IDLE;

    HAL_Delay(200);
    return status;
}

void DbmsIter(DbmsCtx* ctx)
{
	if(ctx->cur_state == DBMS_SHUTDOWN && ctx->req_state == DBMS_ACTIVE){
		ctx->led_state = LED_INIT;
		ProcessLedAction(ctx);
	}

	int status = 0;
    ctx->iterct++;

    //
    //  Store the settings when required
    //
    if (ctx->need_to_sync_settings)
    {
        // should we also check that we are awake?
        // should we allow the user to configure
        // settings while we are "shutdown"
        // even though the controller should be on
        if ((status = SaveSettings(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
            ctx->led_state = LED_ERROR;
        }
        ctx->need_to_sync_settings = false;
    }

    //
    //  Let everybody know that we are alive
    //
    
    CanTxHeartbeat(ctx, CalcCrc16((uint8_t*)&ctx->settings, sizeof(DbmsSettings)));

    //
    //  Its been too long since we have recived a frame, we need to force a shutdown
    //  otherwise we want to be active
    //  
    if (HAL_GetTick() - ctx->last_rx_heartbeat > GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN))
    {
        ctx->req_state = DBMS_SHUTDOWN;
    }
    else {
        ctx->req_state = DBMS_ACTIVE;
    }
    
    // 
    //  Gracefully handle state transition
    //
    if (ctx->cur_state == DBMS_ACTIVE && ctx->req_state == DBMS_SHUTDOWN)
    {
        // on these states -- probably need to write to disk too
        DbmsPerformShutdown(ctx);
    }
    else if (ctx->cur_state == DBMS_SHUTDOWN && ctx->req_state == DBMS_ACTIVE)
    {
        // on these states -- probably need to write to disk too
        DbmsPerformWakeup(ctx);
    }

    //
    //  Read information from the stack
    //
    if (ctx->cur_state == DBMS_ACTIVE)
    {
        // Need to look into this
        // todo: will time out a few times before stack is 
        //       correctly configed, fix this
        StackUpdateVoltReadings(ctx);
    }

    //
    //  Update the LEDs. Led state should be set every time the cur_state changes
    //
    ProcessLedAction(ctx);
    HAL_Delay(10);      // make adaptive
}

void DbmsCanRx(DbmsCtx* ctx, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8])
{
    int status = 0;

    switch (rx_header.StdId)
    {
        case CANID_RX_HEARTBEAT:
            ctx->last_rx_heartbeat = HAL_GetTick();
            break;

        case CANID_RX_SET_CONFIG:
            if ((status = HandleCanConfig(ctx, rx_data, CFG_SET)) != 0)
            {
                if (status == ERR_CFGID_NOT_FOUND) {}   
            }
            break;
        case CANID_RX_GET_CONFIG:
            if ((status = HandleCanConfig(ctx, rx_data, CFG_GET)) != 0)
            {
                if (status == ERR_CFGID_NOT_FOUND) {}   
            }
        default:
            break;
    }
}

//
//  Handle fatal HAL error
//
void DbmsErr(DbmsCtx* ctx)
{
    ctx->cur_state = DBMS_SHUTDOWN;
    ctx->led_state = LED_ERROR;
}

void DbmsClose(DbmsCtx* ctx)
{
    ctx->cur_state = DBMS_SHUTDOWN;
    ctx->led_state = LED_IDLE;
}

