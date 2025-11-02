/** 
 * 
 * Distributed BMS      DBMS Main Function
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#include "dbms.h"
#include "context.h"
#include "ledctl.h"
#include "can.h"
#include "blackbox.h"
#include <math.h>


static DbmsSettings mem_settings;

//
//  Allocate space for pointer objects
//
void DbmsAlloc(DbmsCtx* ctx)
{
    ctx->settings = &mem_settings;
    memset(&ctx->stats, 0, sizeof(ctx->stats));
    BlackboxInit(ctx);
}

//
//  Initialization loop
//
void DbmsInit(DbmsCtx* ctx)
{
    int status = 0;
    ctx->active = false;
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
            ctx->need_to_sync_settings = true; // queue up a write
        }
        else
        {
        } // fatal error
    }

    HAL_TIM_Base_Start(ctx->hw.timer);

    if ((status = ConfigCan(ctx)) != HAL_OK)
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return;
    }

    //
    //  Load the fault state (we know this throws CRC mismatch)
    //
    if ((status = LoadFaultState(ctx)) != 0)
    {
        // CanLog(ctx, "fault st %d\n", status);
    }

    //
    //  Load the initial charge (we know this throws CRC mismatch)
    //
    if ((status = LoadQStats(ctx)) != 0)
    {
        // CanLog(ctx, "error loading initial charge %d\n", status);
    }   
    ctx->need_to_reset_qstats = false;

    ctx->last_rx_heartbeat = -GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);

    ConfigPwmLines(ctx);
    DataInit(ctx);
}

int DbmsPerformWakeup(DbmsCtx* ctx)
{
    int status = 0;
    HAL_Delay(2000);

    // TODO: check these all and make a critical couldnt wake stack fault
    if ((status = StackWake(ctx)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        ctx->led_state = LED_ERROR;
        return status; // we are cooked
    }

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

    // Bridge_Dev_Conf_FAULT_EN(ctx);
    // DelayUs(ctx, 5000);
    // Stack_Dev_Conf_FAULT_EN(ctx);

    DelayUs(ctx, 5000);
    SetFaultMasks(ctx);

    ctx->wakeup_ts = HAL_GetTick();
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
    ctx->led_state = LED_IDLE;

    ctx->qstats.historic_accumulated_loss += ctx->qstats.accumulated_loss;
    ctx->qstats.accumulated_loss = 0;
    CanLog(ctx, "QD = %d\n", (uint32_t)(ctx->qstats.historic_accumulated_loss * 1000));
    SaveQStats(ctx);

    HAL_Delay(200);
    return status;
}

/**
 * Handle active logic: poll voltages and temps, handle faults, etc.
 */
void DbmsHandleActive(DbmsCtx* ctx)
{
    if (!ctx->active) return;

    for (int i = 0; i < N_SIDES; i++)
    {
        #if SPLIT_STACK_OPS
        if (ctx->stats.iters % 2 == i % 2)
        #endif
        {
            StackUpdateVoltReadingSingle(ctx, i);   
            HAL_Delay(SINGLE_MSG_DELAY);            
        }
    }        
    
    HAL_Delay(GROUP_MSG_DELAY);

    for (uint8_t i = 0; i < N_SIDES; i++)
    {
        // todo: think abt switching these
        #if SPLIT_STACK_OPS 
        StackUpdateTempReadingSingle(ctx, i, ctx->stats.iters % 2 == 0);
        HAL_Delay(SINGLE_MSG_DELAY);
        #else
        StackUpdateTempReadingSingle(ctx, i, false);
        HAL_Delay(SINGLE_MSG_DELAY);
        StackUpdateTempReadingSingle(ctx, i, true);
        HAL_Delay(SINGLE_MSG_DELAY);
        #endif
    }

    if (GetSetting(ctx, IGNORE_BAD_THERMS))
    {
        FillMissingTempReadings(ctx);
    }
    HAL_Delay(GROUP_MSG_DELAY);


    if (HAL_GetTick() - ctx->wakeup_ts > GetSetting(ctx, MS_BEFORE_FAULT_CHECKS)) 
    {               
        CheckCurrentFaults(ctx);
        CheckTemperatureFaults(ctx);
        CheckVoltageFaults(ctx);

        // PollFaultSummary(ctx);
        HAL_Delay(SINGLE_MSG_DELAY);
    }

    ThrowHardFault(ctx);                // this can override fault state
    HAL_Delay(GROUP_MSG_DELAY);
}


void DbmsIter(DbmsCtx* ctx)
{
    int status = 0;
    ctx->stats.iters++;
    ctx->iter_start_us = GetUs(ctx);
    
    /**
     * Handle blackbox data requested
     */
    if (ctx->blackbox.requested)
    {
        if ((status = BlackboxSend(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
        }
        ctx->blackbox.requested = false;
    }

    if (ctx->stats.iters % 10 == 0)
    {
        ConfigCurrentSensor(ctx, 10);
    }

    // Store the settings when required
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

    // Store the Q0 value when required
    if (ctx->need_to_reset_qstats)
    {
        if ((status = SaveQStats(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
            ctx->led_state = LED_ERROR;
        }
        ctx->need_to_reset_qstats = false;
    }

    // Let everybody know that we are alive
    CanTxHeartbeat(ctx, CalcCrc16((uint8_t*)ctx->settings, sizeof(DbmsSettings)));

    /**
     * Active/shutdown switch based on main heartbeat
     * If it's been too long since we have recived a frame, we need to force a shutdown
     * otherwise we want to be active
     */
    uint64_t cur_time = HAL_GetTick();
    uint32_t quiet_ms = GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN);

    if (cur_time - ctx->last_rx_heartbeat < quiet_ms)
    {
        if (!ctx->active)
        {
            ctx->led_state = LED_INIT;
            ProcessLedAction(ctx);
            DbmsPerformWakeup(ctx);
            ctx->led_state = LED_ACTIVE;
        }
        ctx->active = true;
        DbmsHandleActive(ctx);
    }
    else 
    {
        if (ctx->active)
            DbmsPerformShutdown(ctx);
        ctx->active = false;
        SetFaultLine(ctx, true);
        ctx->need_to_save_faults = false;
        ctx->led_state = LED_IDLE;
    }


    ChargingUpdate(ctx);

    // Update the state of charge model
    UpdateModel(ctx);   // TODO: add condition for when we update this

    // Blackbox handler
    BlackboxSwapAndUpdate(ctx);

    /**
     * Save faults and blackbox data to eeprom
     */
    if (ctx->need_to_save_faults)
    {
        if ((status = SaveFaultState(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
        }
        if ((status = BlackboxSaveOnFault(ctx, ctx->blackbox.old_data, ctx->blackbox.new_data)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
        }
        ctx->need_to_save_faults = false;
    }

    /**
     * Transmit telemetry
     */
    SendPlexMetrics(ctx);
    if (HAL_GetTick() - ctx->last_rx_telembeat < 5000)// < GetSetting(ctx, QUIET_MS_BEFORE_SHUTDOWN))
    {
        SendMetrics(ctx);               // TODO: resolve conflicting metrics
        SendCellVoltages(ctx);
        SendCellTemps(ctx);
    }

    /**
     * Handle LED states and such
     */
    SetPwmStates(ctx);
    ProcessLedAction(ctx);
    MonitorLedBlink(ctx);

    /**
     * Schedule the next loop
     */
    ctx->iter_end_us = GetUs(ctx);
    ctx->stats.looptime = ctx->iter_end_us - ctx->iter_start_us;
    ctx->stats.end_delay = CalcIterDelay(ctx, ITER_TARGET_HZ);
    
    HAL_Delay(ctx->stats.end_delay / 1000);
    DelayUs(ctx, ctx->stats.end_delay % 1000);
}

void DbmsCanRx(DbmsCtx* ctx, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8])
{
    int status = 0;
    uint32_t can_id = (rx_header.IDE == CAN_ID_EXT) ? rx_header.ExtId : rx_header.StdId;
    ctx->stats.n_rx_can_frames++;

    switch (can_id)
    {
    case CANID_RX_HEARTBEAT:

        ctx->last_rx_heartbeat = HAL_GetTick();
        
        uint64_t remote_ts = be64_to_u64(rx_data);
        SyncRealTime(ctx, remote_ts);

        break;
    case CANID_RX_TELEMBEAT:
        ctx->last_rx_telembeat = HAL_GetTick();
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
        CtrlClearAllFaults(ctx);
        break;

    case CANID_RX_SET_INITIAL_CHARGE:
        ctx->qstats.initial = be32_to_u32(rx_data) / 1e6f;
        CanLog(ctx, "Q0: %d", be32_to_u32(rx_data));
        ctx->qstats.accumulated_loss = 0;
        ctx->qstats.historic_accumulated_loss = 0;
        // ctx->qstats.initial_set_ts = (int32_t)(GetRealTime(ctx) / 1000);
        ctx->qstats.initial_set_ts = 0;
        ctx->need_to_reset_qstats = true;
        break;

    case CANID_RX_BLACKBOX_REQUEST:
        ctx->blackbox.requested = true;
        break;
    case CANID_RX_BLACKBOX_SAVE:
        ctx->need_to_save_faults = true;
        break;

// TODO: remove this
#ifdef DEBUG_DO_OVERWRITE_TEMPS_OVER_CAN
    case CANID_DEBUG_OVERWRITE_TEMPS:
        uint32_t temp_milli_deg_C = be32_to_u32(rx_data);
        float temp_deg_C = temp_milli_deg_C / 1000.0;
        for (uint8_t i = 0; i < N_SIDES; i++)
        {
            for (uint8_t j = 0; j < N_TEMPS_PER_SIDE; j++)
                ctx->cell_states[i].temps[j] = temp_deg_C;
        }
        break;
#endif

    case CANID_ELCON_RX:
        HandleElconHeartbeat(ctx, rx_data);
        break;
    case CANID_CHARGING_HB:
        CanLog(ctx, "Charging HB\n");
        ctx->charging.heartbeat = HAL_GetTick();
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
    ctx->led_state = LED_ERROR;
    DbmsPerformShutdown(ctx);    
}

void DbmsClose(DbmsCtx* ctx)
{
    ctx->led_state = LED_IDLE;
}
