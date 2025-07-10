//  
//  Copyright (c) Texas A&M University.
//  
#include "current_meter.h"

double PollAdc(DbmsCtx* ctx)
{
    HAL_ADC_Start(ctx->hw.adc);
    HAL_ADC_PollForConversion(ctx->hw.adc, 1000);   // poll for conversion
    uint32_t adc_val = HAL_ADC_GetValue(ctx->hw.adc); // get the adc value
    HAL_ADC_Stop(ctx->hw.adc);                      // stop adc
    return (adc_val / 4096.0) * 3.3;
}