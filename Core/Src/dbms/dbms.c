//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"

void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    int status = 0;

    // Manually set state for now
    ctx->state = DBMS_ACTIVE;

    // Initialize cell_states
//    memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(hw->timer);
    status = ConfigCan(hw);

    if (ctx->state == DBMS_ACTIVE)
    {
        LedSet(LED6, LED_GREEN);
        LedSet(LED7, LED_GREEN);
        LedSet(LED8, LED_GREEN);
        HAL_Delay(2000);
        status = StackWake(hw);
        StackAutoAddr(hw);
        StackSetNumActiveCells(hw, 0x0A);
        StackSetupGpio(hw);
        StackSetupVoltReadings(hw);     // todo: rn start
    }
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{
	int status = 0;
    ctx->iterct++;
    
    LedSet(LED6, ((ctx->iterct / 200) % 2 == 0) ? LED_GREEN : LED_OFF);

    if (ctx->state == DBMS_SHUTDOWN) {
        LedSet(LED6, LED_RED);
        LedSet(LED7, LED_RED);
        LedSet(LED8, LED_RED);
        status = StackShutdown(hw);
        HAL_Delay(200);
        return;
    }
    // TEST 2: Try sending CAN messages
    // uint8_t can_frame[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x08};
    // CanTransmit(hw, 0xf5, can_frame);

    // TEST 3: Read voltages
    // StackUpdateVoltReadings(hw, ctx);
    // Log the 5th voltage from the 1st monitor in the 1st segment
    // CanLog(hw, "v=%d", ctx->cell_states[0][0].voltages[4]);

    HAL_Delay(10);
}

void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{

}

void DbmsClose(DbmsCtx* ctx, HwCtx* hw)
{
}

