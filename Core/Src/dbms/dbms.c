//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"
#include "context.h"

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
    ctx->charging.state = NOT_CHARGING;

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

    if ((status = LoadFaultState(ctx)) != 0)
    {
        // todo: check error 
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

    ConfigPwmLines(ctx);

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
    StackBalancingConfig(ctx);

   if ((status = LoadStoredObject(ctx, EEPROM_CTRL_FAULT_MASK_ADDR, &ctx->faults, sizeof(ctx->faults))))
    {
        // todo: check an error here
    }
    if ((status = LoadFaultState(ctx)) != 0)
    {
        // todo: check an error here
    }

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
	// if (ctx->cur_state == DBMS_SHUTDOWN && ctx->req_state == DBMS_ACTIVE)
    // {
	// 	ctx->led_state = LED_INIT;
	// 	ProcessLedAction(ctx);
	// }

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
        ctx->led_state = LED_INIT;
		ProcessLedAction(ctx);
        DbmsPerformWakeup(ctx);
    }

    //
    //   Main State Dispatch
    //
    if (ctx->cur_state == DBMS_ACTIVE)
    {
        //
        //  Communicate with the stack
        //
        StackUpdateVoltReadings(ctx);
        // StackUpdateTempReadings(ctx);
        HAL_Delay(8);
        StackUpdateFaultReadings(ctx);

        if(ctx->charging.state == CHARGING_ACTIVE){
            if(StackNeedsBalancing(ctx)){
                // pause charger via can, enter balancing mode
                ctx->charging.state = CHARGING_PAUSED_FOR_BALANCING;
            }
        }
        else if(ctx->charging.state == CHARGING_PAUSED_FOR_BALANCING){

            if(StackBalancingAbortedByFault(ctx)){
                ctx->charging.state = CHARGING_ERROR;
                ctx->led_state = LED_ERROR;

                HAL_Delay(100);
                DbmsPerformShutdown(ctx);
                
            }
            if(StackBalancingComplete(ctx)){
                // resume charger via can, exit balancing mode
                ctx->charging.state = CHARGING_ACTIVE;
            }
        }

        //
        //  Check fault conditions
        //
        CheckVoltageFaults(ctx);
        // CheckTemperatureFaults(ctx);
        CheckCurrentFaults(ctx);
    }

    // 
    //  If there is a hard fault we are shutting off the car
    //
    ThrowHardFault(ctx);

    //
    //  Transmit important telemetry 
    //
    SendMetrics(ctx);
    if (ctx->iter_start_us - ctx->batch_telem_ts > 10000) 
    {
        ctx->batch_telem_ts = ctx->iter_start_us;
        SendCellVoltages(ctx);
        SendCellTemps(ctx);
    }

    ctx->model.soc = (ctx->stats.iters % 100) / 100.0;

    //
	//  Update the LEDs (etc.). Led state should be set every time the cur_state changes
	//
    SetPwmStates(ctx);
    ProcessLedAction(ctx);
    MonitorLedBlink(ctx);

    //
    //  Schedule the next loop
    //
    ctx->iter_end_us = GetUs(ctx);
    ctx->stats.looptime = ctx->iter_end_us - ctx->iter_start_us;
    ctx->stats.end_delay = CalcIterDelay(ctx, ITER_TARGET_HZ);
    // DelayUs(ctx, end_delay);
    HAL_Delay(8);  // ^ todo: fix all ts
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
        case CANID_RX_CLEAR_FAULTS:
            ClearAllFaults(ctx);
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

