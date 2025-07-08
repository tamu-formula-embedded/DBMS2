//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"


// TODO:    replace this with an implementation 
//          that makes sense, either EEPROM or FLASH
void LoadSettings(DbmsCtx* ctx, HwCtx* hw)
{
    ctx->settings.max_allowed_pack_voltage = 64000; // 64V -> mV
    ctx->settings.quiet_ms_before_shutdown = 5000;  // 5s -> ms
}


void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    int status = 0;
    ctx->cur_state = DBMS_SHUTDOWN;

    LoadSettings(ctx, hw);

    //memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(hw->timer);
    status = ConfigCan(hw);
}

int DbmsPerformWakeup(DbmsCtx* ctx, HwCtx* hw)
{
    int status = 0;
    HAL_Delay(2000);

    if ((status = StackWake(hw)) != 0)
    {
        CAN_REPORT_FAULT(hw, status);
        LED_SHOW_ERROR();
        return status;                  // we are cooked
    }

    LedSet(LED6, LED_GREEN);
    LedSet(LED7, LED_GREEN);
    LedSet(LED8, LED_GREEN);

    StackAutoAddr(hw);
    StackSetNumActiveCells(hw, 0x0A);
    StackSetupGpio(hw);
    StackSetupVoltReadings(hw);     // todo: rn start

    ctx->cur_state = DBMS_ACTIVE;
    return status;
}

int DbmsPerformShutdown(DbmsCtx* ctx, HwCtx* hw)
{
    int status = 0;
    if ((status = StackShutdown(hw)) != 0)
    {
        CAN_REPORT_FAULT(hw, status);
        LED_SHOW_ERROR();
        return status;
    }

    LedSet(LED6, LED_OFF);
    LedSet(LED7, LED_OFF);
    LedSet(LED8, LED_OFF);

    HAL_Delay(200);
    return status;
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{
	int status = 0;
    ctx->iterct++;
    
    //
    //  Let everybody know that we are alive
    //
    LedSet(LED6, ((ctx->iterct / 20) % 2 == 0) ? LED_GREEN : LED_OFF);
    CanTxHeartbeat(hw, CalcCrc16((uint8_t*)&ctx->settings, sizeof(DbmsSettings)));

    //
    //  Its been too long since we have recived a frame, we need to force a shutdown
    //  otherwise we want to be active
    //  
    if (HAL_GetTick() - ctx->last_rx_heartbeat > ctx->settings.quiet_ms_before_shutdown)
        ctx->req_state = DBMS_SHUTDOWN;
    else 
        ctx->req_state = DBMS_ACTIVE;

    
    // 
    //  Gracefully handle state transition
    //
    if (ctx->cur_state == DBMS_ACTIVE && ctx->req_state == DBMS_SHUTDOWN)
    {
        DbmsPerformShutdown(ctx, hw);
    }
    else if (ctx->cur_state == DBMS_SHUTDOWN && ctx->req_state == DBMS_ACTIVE)
    {
        DbmsPerformWakeup(ctx, hw);
    }


    HAL_Delay(10);
}

void DbmsCanRx(DbmsCtx* ctx, HwCtx* hw, CanRxChannel channel, CAN_RxHeaderTypeDef rx_header, uint8_t rx_data[8])
{
    // CanLog(hw, "rx %d", channel);
    // CanTransmit(hw, 0x02, rx_data);

    switch (rx_header.StdId)
    {
        case CANID_RX_HEARTBEAT:
            ctx->last_rx_heartbeat = HAL_GetTick();
            break;

        case CANID_RX_CONFIG:
            HandleCanConfig(hw, ctx, rx_data);
            break;

        default:
            break;
    }
}

//
//  Handle fatal HAL error
//
void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{
    
}

void DbmsClose(DbmsCtx* ctx, HwCtx* hw)
{
}

