#include "fan.h"

int InitFan(DbmsCtx* ctx)
{
    HAL_TIM_PWM_Start(ctx->hw.timer_pwm_1, TIM_CHANNEL_4);
    return 0;
}

int UpdateFan(DbmsCtx* ctx)
{
    int duty = 0;

    if (ctx->stats.avg_t > GetSetting(ctx, FAN_T_TH))
    {
        duty = 20;
    }
    CanLog(ctx, 
            "Fan Setting: %d ; Duty: %d ; DELTA: %d ; MIN_V: %d ; T_IDX: %d ; TARGET_V: %d\n",
            GetSetting(ctx, FAN_T_TH), 
            duty, 
            GetSetting(ctx, CH_BAL_DELTA), 
            GetSetting(ctx, CH_BAL_MIN_V),
            GetSetting(ctx, CH_BAL_T_IDX),
            GetSetting(ctx, CH_TARGET_V));

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(ctx->hw.timer_pwm_1);
    uint32_t ccr = 0;

    if (duty <= 0)
    {
        ccr = 0;
    }
    else if (duty >= 100)
    {
        ccr = arr;
    }
    else
    {
        ccr = (uint32_t)((duty * (arr + 1U)) / 100U);
        if (ccr > arr) ccr = arr;
    }

    __HAL_TIM_SET_COMPARE(ctx->hw.timer_pwm_1, TIM_CHANNEL_4, ccr);

    return 0;
}