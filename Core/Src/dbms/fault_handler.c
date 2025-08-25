#include "fault_handler.h"
#include "settings.h"

void SetFault(DbmsCtx* ctx, FaultType fault) 
{
    if (fault >= FAULT_TYPE_COUNT) return;
    ctx->fault_mask |= (1U << fault);
}

void ClearFault(DbmsCtx* ctx, FaultType fault)
{
    if (fault >= FAULT_TYPE_COUNT) return;
    ctx->fault_mask &= ~(1U << fault);
}

bool HasFault(DbmsCtx* ctx, FaultType fault)
{
    if (fault >= FAULT_TYPE_COUNT) return false;
    return (ctx->fault_mask & (1U << fault)) != 0;
}

void CheckVoltageFaults(DbmsCtx* ctx)
{
    uint32_t max_group_v = GetSetting(ctx, MAX_GROUP_VOLTAGE);
    uint32_t min_group_v = GetSetting(ctx, MIN_GROUP_VOLTAGE);

    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            if (ctx->cell_states[i].voltages[j] > max_group_v)
            {
                SetFault(ctx, FAULT_VOLTAGE_OVER);
            }
            if (ctx->cell_states[i].voltages[j] < min_group_v)
            {
                SetFault(ctx, FAULT_VOLTAGE_UNDER);
            }
        }
    }
    
    /*
    if (ctx->isense.voltage1_mv > GetSetting(ctx, MAX_PACK_VOLTAGE))
    {
        SetFault(ctx, FAULT_PACK_VOLTAGE_OVER);
    }
    if (ctx->isense.voltage1_mv < GetSetting(ctx, MIN_PACK_VOLTAGE))
    {
        SetFault(ctx, FAULT_PACK_VOLTAGE_UNDER);
    }
    */
}

void CheckTemperatureFaults(DbmsCtx* ctx)
{
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
            if (ctx->cell_states[i].temps[j] > GetSetting(ctx, MAX_THERMISTOR_TEMP))
            {
                SetFault(ctx, FAULT_TEMP_OVER);
            }
        }
    }
}

void CheckCurrentFaults(DbmsCtx* ctx)
{
    if (ctx->isense.current_ma > GetSetting(ctx, MAX_CURRENT))
    {
        SetFault(ctx, FAULT_CURRENT_OVER);
    }
}