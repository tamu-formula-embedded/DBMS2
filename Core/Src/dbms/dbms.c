//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"


void LoadFallbackSettings(DbmsSettings* settings)
{
    // if this fails we are in deep trouble lol
    settings->max_allowed_pack_voltage = 64000; // 64V -> mV
    settings->quiet_ms_before_shutdown = 5000;  // 5s -> ms
}


void DbmsInit(DbmsCtx* ctx)
{
    int status = 0;
    ctx->cur_state = DBMS_SHUTDOWN;

    if ((status = LoadSettings(ctx, &ctx->settings)) != HAL_OK)
    {
        CAN_REPORT_FAULT(ctx, status);
        if (status == ERR_CRC_MISMATCH)
        {
            // This is a bad situation, but we can still proceed
            // by loading fallback settings? Ideally these should
            // be updated enough to be ok
            LoadFallbackSettings(&ctx->settings);
            ctx->need_to_sync_settings = true;          // queue up a write
        }  
        else    {}  // fatal error
}
    //memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(ctx->hw.timer);
    status = ConfigCan(ctx);
}

int DbmsPerformWakeup(DbmsCtx* ctx)
{
    int status = 0;
    HAL_Delay(2000);

    if ((status = StackWake(ctx)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        return status;                  // we are cooked
    }

    LedSet(LED6, LED_GREEN);
    LedSet(LED7, LED_GREEN);
    LedSet(LED8, LED_GREEN);

    StackAutoAddr(ctx);
    StackSetNumActiveCells(ctx, 0x0A);
    StackSetupGpio(ctx);
    StackSetupVoltReadings(ctx);     // todo: rn start

    ctx->cur_state = DBMS_ACTIVE;
    return status;
}

int DbmsPerformShutdown(DbmsCtx* ctx)
{
    int status = 0;
    if ((status = StackShutdown(ctx)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        return status;
    }

    LedSet(LED6, LED_OFF);
    LedSet(LED7, LED_OFF);
    LedSet(LED8, LED_OFF);

    HAL_Delay(200);
    return status;
}

void DbmsIter(DbmsCtx* ctx)
{
	int status = 0;
    ctx->iterct++;

    if (ctx->need_to_sync_settings)
    {
        // should we also check that we are awake?
        // should we allow the user to configure
        // settings while we are "shutdown"
        // even though the controller should be on
        if ((status = SaveSettings(ctx, &ctx->settings)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
        }
        ctx->need_to_sync_settings = false;
    }
    
    //
    //  Let everybody know that we are alive
    //
    if (ctx->led_show_error) 
    {
        LED_SHOW_ERROR();
    } else {
        LedSet(LED6, ((ctx->iterct / 20) % 2 == 0) ? LED_GREEN : LED_OFF);
    }
    CanTxHeartbeat(ctx, CalcCrc16((uint8_t*)&ctx->settings, sizeof(DbmsSettings)));

    //
    //  Its been too long since we have recived a frame, we need to force a shutdown
    //  otherwise we want to be active
    //  
    // if (HAL_GetTick() - ctx->last_rx_heartbeat > ctx->settings.quiet_ms_before_shutdown)
        // ctx->req_state = DBMS_SHUTDOWN;
    // else 
        ctx->req_state = DBMS_ACTIVE;

    
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


    HAL_Delay(10);
}

void DbmsCanRx(DbmsCtx* ctx, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8])
{
    // CanLog(ctx, "rx %d", channel);
    // CanTransmit(ctx, 0x02, rx_data);

    switch (rx_header.StdId)
    {
        case CANID_RX_HEARTBEAT:
            ctx->last_rx_heartbeat = HAL_GetTick();
            break;

        case CANID_RX_CONFIG:
            int status = HandleCanConfig(ctx, rx_data);
            if (status != ERR_CFGID_NOT_FOUND)
            {
                ctx->need_to_sync_settings = true;
            }
            break;

        default:
            break;
    }
}

//
//  Handle fatal HAL error
//
void DbmsErr(DbmsCtx* ctx)
{
    
}

void DbmsClose(DbmsCtx* ctx)
{
}

