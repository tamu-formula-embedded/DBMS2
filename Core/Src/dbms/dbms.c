//  
//  Copyright (c) Texas A&M University.
//  
#include "dbms.h"



void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    HAL_TIM_Base_Start(hw->timer);
    ConfigCan(hw);

    HAL_Delay(2000); 

    StackAutoAddr(hw);
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{

    CanLog(hw, "Sensor = %f\n", PollAdc(hw));

    HAL_Delay(1);
}

void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{

}

void DbmsClose(DbmsCtx* ctx, HwCtx* hw)
{
}