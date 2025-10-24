#include "faults.h"
#include "../blackbox.h"
#include "../settings.h"

void CtrlSetFault(DbmsCtx* ctx, CtrlFault fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return;
    ctx->faults.controller_mask |= (1U << fault);
}

void CtrlClearFault(DbmsCtx* ctx, CtrlFault fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return;
    ctx->faults.controller_mask &= ~(1U << fault);
}

bool CtrlHasFault(DbmsCtx* ctx, CtrlFault fault)
{
    if (fault >= CTRL_FAULT_TYPE_COUNT) return false;
    return (ctx->faults.controller_mask & (1U << fault)) != 0;
}

bool CtrlHasAnyFaults(DbmsCtx* ctx)
{
    if (ctx->faults.controller_mask != 0) return true;
    return false;
}

void CtrlClearAllFaults(DbmsCtx* ctx)
{
    ctx->faults.controller_mask = 0;
    ctx->need_to_save_faults = true;
}

void SetFaultLine(DbmsCtx* ctx, bool faulted)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, !((bool)faulted));
    ctx->stats.fault_line_faulted = faulted;
}

void ThrowHardFault(DbmsCtx* ctx)
{
    if (CtrlHasAnyFaults(ctx)) 
    {
        ctx->led_state = LED_FAULT;
        // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
        SetFaultLine(ctx, true);
    }
    else 
    {
        // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);
        SetFaultLine(ctx, false);
    }
    
    if (!ctx->faults.had_fault && CtrlHasAnyFaults(ctx))
    {
        ctx->need_to_save_faults = true;
    }
    ctx->faults.had_fault = CtrlHasAnyFaults(ctx);
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
    // Right now this only references the controller mask. When "smart" / "verbose" faults 
    // for other components on the bus are implemented, we will deal with this.
    if ((status = LoadStoredObject(ctx, EEPROM_CTRL_FAULT_MASK_ADDR, 
            &(ctx->faults.controller_mask), sizeof(ctx->faults.controller_mask))))
    {
        // todo: check an error here
    }
    ctx->faults_crc = CalcCrc16((uint8_t*)&ctx->faults, sizeof(ctx->faults));
    ctx->faults.had_fault = CtrlHasAnyFaults(ctx);
    return status;
}
