#include "j1772.h"

void J1772SetChargeEnable(DbmsCtx* ctx, bool en)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, en);
    ctx->j1772.charge_en_req = en;
}

void J1772ReadState(DbmsCtx* ctx)
{
    // Legacy: read PP digital in
    // ctx->j1772.pp_connect = (bool)HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

    // Read PP as analog and decide if connected or disconnected by voltage level
    HAL_ADC_Start(ctx->hw.adc);
    HAL_ADC_PollForConversion(ctx->hw.adc, 1000);           // poll for conversion
    uint32_t adc_val = HAL_ADC_GetValue(ctx->hw.adc);       // get the adc value
    HAL_ADC_Stop(ctx->hw.adc);                              // stop adc
    ctx->j1772.pp_raw_voltage = (adc_val * 3300) / 4096;    // losing a little bit of precision

    ctx->j1772.pp_connect = ctx->j1772.pp_raw_voltage < PP_LOW_THRESHOLD_MV;

    // logic:?

    bool getting_pwm_connection = (HAL_GetTick() - ctx->j1772.last_cp_pwm_read) < PWM_READ_TIMEOUT;

    if (getting_pwm_connection && !ctx->j1772.pwm_recieved)
    {
        ctx->j1772.pwm_ts = HAL_GetTick();
        ctx->j1772.pwm_recieved = true;
        //uint8_t data[8];
        //CanTransmit(ctx, CANID_ELCON_TX_HB, data);
    }
    if (!getting_pwm_connection)
    {
        ctx->j1772.pwm_recieved = false; 
        J1772SetChargeEnable(ctx, false);
    }

    // J1772SetChargeEnable(ctx, HAL_GetTick() - ctx->j1772.pwm_ts > 10000 && ctx->j1772.pwm_recieved);
}
