#include "fault_handler.h"
#include "settings.h"

void ControllerSetFault(DbmsCtx* ctx, ControllerFaultType fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return;
    ctx->faults.controller_mask |= (1U << fault);
}

void ControllerClearFault(DbmsCtx* ctx, ControllerFaultType fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return;
    ctx->faults.controller_mask &= ~(1U << fault);
}

bool ControllerHasFault(DbmsCtx* ctx, ControllerFaultType fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return false;
    return (ctx->faults.controller_mask & (1U << fault)) != 0;
}

void BridgeSetFaultSummary(DbmsCtx* ctx, uint8_t fault_summary_reg)
{
    ctx->faults.bridge_fault_summary = (fault_summary_reg & 0x0F); // Bits [5:8] are reserved
}

void BridgeSetFault(DbmsCtx* ctx, BridgeFault fault)
{
    if (fault >= BRIDGE_FAULT_TYPE_COUNT) return;
    ctx->faults.bridge_faults |= (1U << fault);
}

void BridgeClearFault(DbmsCtx* ctx, BridgeFault fault)
{
    if (fault >= BRIDGE_FAULT_TYPE_COUNT) return;
    ctx->faults.bridge_faults &= ~(1U << fault);
}

bool BridgeHasFault(DbmsCtx* ctx, BridgeFault fault)
{
    if (fault >= BRIDGE_FAULT_TYPE_COUNT) return false;
    return (ctx->faults.bridge_faults & (1U << fault)) != 0;
}

void StackSetFaultSummary(DbmsCtx* ctx, uint8_t addr, uint8_t fault_summary_reg){
    ctx->faults.monitor_fault_summary[addr] = fault_summary_reg;
}

bool HasAnyFaults(DbmsCtx* ctx)
{
    if (ctx->faults.controller_mask != 0) return true;
    if (ctx->faults.bridge_fault_summary != 0) return true;
    for (int i = 0; i < N_MONITORS; i++)
    {
        if (ctx->faults.monitor_fault_summary[i] != 0) return true;
    }
    return false;
}

void ClearAllFaults(DbmsCtx* ctx)
{
    ctx->faults.controller_mask = 0;
    ctx->faults.bridge_fault_summary = 0;
    ctx->faults.bridge_faults = 0;
    for (int i = 0; i < N_MONITORS; i++)
    {
        ctx->faults.monitor_fault_summary[i] = 0;
    }
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
                ControllerSetFault(ctx, CTRL_FAULT_VOLTAGE_OVER);
            }
            if (ctx->cell_states[i].voltages[j] < min_group_v)
            {
                ControllerSetFault(ctx, CTRL_FAULT_VOLTAGE_UNDER);
            }
        }
    }

    /*
    if (ctx->isense.voltage1_mv > GetSetting(ctx, MAX_PACK_VOLTAGE))
    {
        ControllerSetFault(ctx, CTRL_FAULT_PACK_VOLTAGE_OVER);
    }
    if (ctx->isense.voltage1_mv < GetSetting(ctx, MIN_PACK_VOLTAGE))
    {
        ControllerSetFault(ctx, CTRL_FAULT_PACK_VOLTAGE_UNDER);
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
                ControllerSetFault(ctx, CTRL_FAULT_TEMP_OVER);
            }
        }
    }
}

void CheckCurrentFaults(DbmsCtx* ctx)
{
    if (ctx->isense.current_ma > GetSetting(ctx, MAX_CURRENT))
    {
        ControllerSetFault(ctx, CTRL_FAULT_CURRENT_OVER);
    }
}

void ThrowHardFault(DbmsCtx* ctx)
{
    if (HasAnyFaults(ctx)) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);
    else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);

    SaveFaultState(ctx);
}

int SaveFaultState(DbmsCtx* ctx)
{
    uint16_t faults_crc = CalcCrc16((uint8_t*)&ctx->faults, sizeof(ctx->faults));
    if (ctx->faults_crc != faults_crc)
    {
        ctx->faults_crc = faults_crc;
        SaveStoredObject(ctx, EEPROM_CTRL_FAULT_MASK_ADDR, &ctx->faults, sizeof(ctx->faults));
    }
    return 0;
}

int LoadFaultState(DbmsCtx* ctx)
{
    int status;
    if ((status = LoadStoredObject(ctx, EEPROM_CTRL_FAULT_MASK_ADDR, &ctx->faults, sizeof(ctx->faults))))
    {
        // todo: check an error here
    }
    ctx->faults_crc = CalcCrc16((uint8_t*)&ctx->faults, sizeof(ctx->faults));
    return status;
}
