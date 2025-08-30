//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"

static DbmsSettings mem_settings;

//
//  Allocate space for pointer objects
//
void DbmsAlloc(DbmsCtx* ctx)
{
    ctx->settings = &mem_settings;
    memset(&ctx->stats, 0, sizeof(ctx->stats));
}

//
//  Initialization loop
//
void DbmsInit(DbmsCtx* ctx)
{
    int status = 0;
    ctx->cur_state = DBMS_SHUTDOWN;
    ctx->led_state = LED_INIT;

    // wrap_queue_init(&ctx->stats.looptimes_q, ctx->stats.looptimes_d, N_HISTORIC_LOOPTIMES, sizeof(*ctx->stats.looptimes_d));

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


    HAL_TIM_Base_Start(ctx->hw.timer);

    if ((status = ConfigCan(ctx)) != HAL_OK)
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return;
    }

    HAL_Delay(10);
    ConfigCurrentSensor(ctx, 10);

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

#define PERIOD(CNT, HZ, OFFSET) (fmod(CNT, (ITER_TARGET_HZ/(HZ+0.))) == OFFSET)

void DbmsIter(DbmsCtx* ctx)
{
	int status = 0;
    ctx->stats.iters++;
    ctx->iter_start_us = GetUs(ctx);
    // CanLog(ctx, "%d\n", GetSetting(ctx, MAX_GROUP_VOLTAGE));

	if (ctx->cur_state == DBMS_SHUTDOWN && ctx->req_state == DBMS_ACTIVE)
    {
		ctx->led_state = LED_INIT;
		ProcessLedAction(ctx);
	}
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
    
    CanTxHeartbeat(ctx, CalcCrc16((uint8_t*)ctx->settings, sizeof(DbmsSettings)));

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
//        StackUpdateVoltReadings(ctx);
//        DelayUs(ctx, 20000);
        StackUpdateTempReadings(ctx);
        DelayUs(ctx, 20000);
    }

    // if (PERIOD(ctx->stats.iters, 1, 0)) //todo: fix ts
    SendCellVoltages(ctx);
    SendCellTemps(ctx);
    SendMetrics(ctx);

    //
    //  Example usage: Turn off monitor chip
    //
    // ToggleAllMonitorChipLeds(ctx, false);

    //
	//  Update the LEDs. Led state should be set every time the cur_state changes
	//
    ProcessLedAction(ctx);

    ctx->iter_end_us = GetUs(ctx);

    ctx->stats.looptime = ctx->iter_end_us - ctx->iter_start_us;
    ctx->stats.end_delay = CalcIterDelay(ctx, ITER_TARGET_HZ);

//    MonitorLedBlink(ctx);
    // *((uint32_t*)wrap_queue_push(&ctx->stats.looptimes_q)) = ctx->iter_end_us - ctx->iter_start_us;
    // DelayUs(ctx, end_delay);
    HAL_Delay(20);  // ^ todo: fix all this
}



void DbmsCanRx(DbmsCtx* ctx, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8])
{
    int status = 0;

    ctx->stats.n_rx_can_frames++;

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
            break;
        case CANID_ISENSE_CURRENT:
            ctx->isense.current_ma = (int32_t)UnpackCurrentSensorData(rx_data);
            break;
        case CANID_ISENSE_VOLTAGE1:
            ctx->isense.voltage1_mv = (int32_t)UnpackCurrentSensorData(rx_data);
            break;
        case CANID_ISENSE_POWER:
            ctx->isense.power_w = (int32_t)UnpackCurrentSensorData(rx_data);
            break;
        case CANID_ISENSE_CHARGE:
            ctx->isense.charge_as = (int32_t)UnpackCurrentSensorData(rx_data);
            break;
        case CANID_ISENSE_ENERGY:
            ctx->isense.energy_wh = (int32_t)UnpackCurrentSensorData(rx_data);
            break;
        default:
            ctx->stats.n_unmatched_can_frames++;
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

