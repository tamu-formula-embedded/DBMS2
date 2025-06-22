//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"


void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    memset(ctx->cell_states, 0, N_SEGMENTS * N_MONITORS_PER_SEG * (N_GROUPS * sizeof(int16_t)) * (N_TEMPS * sizeof(int16_t)));

    HAL_TIM_Base_Start(hw->timer);
    ConfigCan(hw);

    // HAL_Delay(2000); 

    StackWake(hw);
    StackAutoAddr(hw);
    StackSetNumActiveCells(hw, 0x0A);
    StackSetupGpio(hw);
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{

    // CanLog(hw, "Sensor = %f\n", PollAdc(hw));

    DumpCellState(ctx, hw);

    HAL_Delay(50);
}

void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{

}

void DbmsClose(DbmsCtx* ctx, HwCtx* hw)
{
}

