#include "faults.h"

void CheckVoltageFaults(DbmsCtx* ctx)
{
    uint32_t max_group_v = GetSetting(ctx, MAX_GROUP_VOLTAGE);
    uint32_t min_group_v = GetSetting(ctx, MIN_GROUP_VOLTAGE);
    uint32_t max_v_delta = GetSetting(ctx, MAX_V_DELTA);
    
    if (ctx->stats.max_v * 1000 > max_group_v) {
        CtrlSetFault(ctx, CTRL_FAULT_VOLTAGE_OVER);
    }
    if (ctx->stats.min_v * 1000 < min_group_v) {
        CtrlSetFault(ctx, CTRL_FAULT_VOLTAGE_UNDER);
    }
    if ((ctx->stats.max_v - ctx->stats.min_v) * 1000 > max_v_delta) {
        CtrlSetFault(ctx, CTRL_FAULT_MAX_DELTA_EXCEEDED);
    }
}

void CheckTemperatureFaults(DbmsCtx* ctx)
{
    if (ctx->stats.max_t < GetSetting(ctx, MAX_THERMISTOR_TEMP))    // temp is ok 
    {
        ctx->timing.overtemp_last_ok_ts = HAL_GetTick();
    }
    else {
        if (HAL_GetTick() - ctx->timing.overtemp_last_ok_ts > GetSetting(ctx, OVERTEMP_MS))
            CtrlSetFault(ctx, CTRL_FAULT_TEMP_OVER);
    }
}

void CheckCurrentFaults(DbmsCtx* ctx)
{
    int32_t current_ma = MAX(0, ctx->current_sensor.current_ma);

    int32_t at = GetSetting(ctx, TEMP_CURVE_A);
    int32_t bt = GetSetting(ctx, TEMP_CURVE_B);

    // Todo: make this cleaner
    if (ctx->stats.avg_t < at) {
        ctx->max_current_ma = GetSetting(ctx, MAX_CURRENT) * 1000;
    } else {
        if (bt == at) ctx->max_current_ma = 0;
        ctx->max_current_ma = (GetSetting(ctx, MAX_CURRENT) * 1000) * ((bt - ctx->stats.avg_t) / (bt - at));
    }

    // Do the comparison in ma
    if (current_ma > ctx->max_current_ma)
    {
        CtrlSetFault(ctx, CTRL_FAULT_CURRENT_OVER);
    }   

    if (current_ma > GetSetting(ctx, PULSE_LIMIT_CURRENT) * 1000)
    {
        ctx->timing.pl_pulse_t = HAL_GetTick() - ctx->timing.pl_last_ok_ts;
        if (ctx->timing.pl_pulse_t > GetSetting(ctx, PULSE_LIMIT_TIME_MS))
            CtrlSetFault(ctx, CTRL_FAULT_CURRENT_PULSE);
    }
    else 
    {
        ctx->timing.pl_pulse_t = 0;
        ctx->timing.pl_last_ok_ts = HAL_GetTick();
    }

    // TODO: need to make ma constructor
    // ctx->current_sensor.ima.current_mavg_ma = ma_calc_i32(ctx->current_sensor.ima.ma, ctx->current_sensor.current_ma);
}
