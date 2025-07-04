//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"

void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    // Manually set state for now
    ctx->state = DBMS_ACTIVE;

    // Initialize cell_states
    memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(hw->timer);
    ConfigCan(hw);

    if (ctx->state == DBMS_ACTIVE)
    {
        // TEST 1: Ensure that these things turn on the stack device.
        HAL_Delay(2000); 
        StackWake(hw);
        StackAutoAddr(hw);
        StackSetNumActiveCells(hw, 0x0A);
        StackSetupGpio(hw);
        StackSetupVoltReadings(hw);     // todo: rn start
    }
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{
    if (ctx->state == DBMS_SHUTDOWN) {
        StackShutdown(hw);
        HAL_Delay(200);
        return;
    }
    // TEST 2: Try sending CAN messages
    uint8_t can_frame[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x08};
    CanTransmit(hw, 0xf5, can_frame);

    HAL_Delay(10);
}

void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{

}

void DbmsClose(DbmsCtx* ctx, HwCtx* hw)
{
}

