//
//  Copyright (c) Texas A&M University.
//
#include "dbms.h"
#include "vehicle_interface.h"
#include <math.h>


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

    if ((status = LoadSettings(ctx)) != HAL_OK)
    {
        CAN_REPORT_FAULT(ctx, status);
        if (status == ERR_CRC_MISMATCH)
        {
            // This is a bad situation, but we can still proceed
            // by loading fallback settings? Ideally these should
            // be updated enough to be ok
            LoadFallbackSettings(ctx);
            ctx->need_to_sync_settings = true; // queue up a write
        }
        else
        {
        } // fatal error
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
    DataInit(ctx);

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
        return status; // we are cooked
    }
    ctx->cur_state = DBMS_ACTIVE;
    ctx->led_state = LED_ACTIVE;

    StackAutoAddr(ctx);
    StackSetNumActiveCells(ctx, 0x0A);
    StackSetupGpio(ctx);
    StackSetupVoltReadings(ctx); // todo: rn start
    StackBalancingConfig(ctx);

    if ((status = LoadStoredObject(ctx, EEPROM_CTRL_FAULT_MASK_ADDR, &ctx->faults, sizeof(ctx->faults))))
    {
        // todo: check an error here
    }
    if ((status = LoadFaultState(ctx)) != 0)
    {
        // todo: check an error here
    }

    // StackStartCharging(ctx);

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

#define PERIOD(CNT, HZ, OFFSET) (fmod(CNT, (ITER_TARGET_HZ / (HZ + 0.))) == OFFSET)

void DbmsIter(DbmsCtx* ctx)
{
    int status = 0;
    ctx->stats.iters++;
    ctx->iter_start_us = GetUs(ctx);

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
    else
    {
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
        StackUpdateVoltReadings(ctx);
        HAL_Delay(8);

        StackUpdateTempReadings(ctx);
        
        // FillMissingTempReadings(ctx);
        CanLog(ctx, "T%d\n", CLAMP_U16((long)lroundf(ctx->cell_states[0].temps[6] * 1000.0f)));
        HAL_Delay(8);

        CanLog(ctx, "first: %d\n", (int)(ctx->data.lut_therm_v_to_t[0].value));

        // StackUpdateFaultReadings(ctx);  // todo: put this first?
    }

    //
    //  Update the state of charge model
    //
    UpdateModel(ctx);   // TODO: add condition for when we update this

    //
    //  Transmit important telemetry
    //
    SendMetrics(ctx);
    if (/*ctx->cur_state == DBMS_ACTIVE && */ ctx->iter_start_us - ctx->batch_telem_ts > 10000)
    {
        ctx->batch_telem_ts = ctx->iter_start_us;
        SendCellVoltages(ctx);
        SendCellTemps(ctx);
    }

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

    // MonitorLedBlink(ctx);
    // *((uint32_t*)wrap_queue_push(&ctx->stats.looptimes_q)) = ctx->iter_end_us - ctx->iter_start_us;
    // DelayUs(ctx, end_delay);
    HAL_Delay(20); // ^ todo: fix all this
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
            if (status == ERR_CFGID_NOT_FOUND)
            {
            }
        }
        break;
    case CANID_RX_GET_CONFIG:
        if ((status = HandleCanConfig(ctx, rx_data, CFG_GET)) != 0)
        {
            if (status == ERR_CFGID_NOT_FOUND)
            {
            }
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
