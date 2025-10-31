#include "pwm.h"

int ConfigPwmLines(DbmsCtx* ctx)
{
    HAL_TIM_PWM_Start(ctx->hw.timer_pwm_1, TIM_CHANNEL_4);
    return 0;
}

int SetPwmStates(DbmsCtx* ctx)
{
    // TODO: make this impl fan control
    float soc = 0;  

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(ctx->hw.timer_pwm_1);
    uint32_t ccr = (uint32_t)((soc / 100.0f) * (arr + 1));

    __HAL_TIM_SET_COMPARE(ctx->hw.timer_pwm_1, TIM_CHANNEL_4, ccr);

    return 0;
}