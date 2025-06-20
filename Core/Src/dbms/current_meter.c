//
//  Copyright (C) Texas A&M University
//
//  imeter -- Current (I) Meter
//
#include "current_meter.h"

double PollAdc(HwCtx* hw_ctx)
{
    HAL_ADC_Start(hw_ctx->adc);
    HAL_ADC_PollForConversion(hw_ctx->adc, 1000);   // poll for conversion
    uint32_t adc_val = HAL_ADC_GetValue(hw_ctx->adc); // get the adc value
    HAL_ADC_Stop(hw_ctx->adc);                      // stop adc
    return (adc_val / 4096.0) * 3.3;
}