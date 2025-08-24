#include "fault_handler.h"
#include "settings.h"

/**
 * Sets a specific fault bit in the mask
*/
void SetFault(DbmsCtx* ctx, FaultType fault){
    if(fault < FAULT_TYPE_COUNT){
        ctx->fault_mask |= (1U << fault);
    }
}

/**
 * Clears a specific fault bit in the mask
*/
void ClearFault(DbmsCtx* ctx, FaultType fault){
    if(fault < FAULT_TYPE_COUNT){
        ctx->fault_mask &= ~(1U << fault);
    }
}

/**
 * Checks if a specific fault bit is set in the mask
*/
bool HasFault(DbmsCtx* ctx, FaultType fault){
    if(fault < FAULT_TYPE_COUNT){
        return (ctx->fault_mask & (1U << fault)) != 0;
    }
    return false;
}

/**
 * Checks if any fault bits are set in the mask
*/
bool HasAnyFaults(DbmsCtx* ctx){
    return ctx->fault_mask != 0;
}

/**
 * grabs the current fault mask from ctx
*/
uint32_t GetFaultMask(DbmsCtx* ctx){
    return ctx->fault_mask;
}

/**
 * Clears all fault bits in the mask
*/
void ClearAllFaults(DbmsCtx* ctx){
    ctx->fault_mask = 0;
}

// Fault Checking Functions

void CheckVoltageFaults(DbmsCtx* ctx){
    for(int i = 0; i < N_SIDES; ++i){
        for(int j = 0; j < N_GROUPS_PER_SIDE; ++j){
            if(ctx->cell_states[i].voltages[j] > GetSetting(ctx, MAX_GROUP_VOLTAGE)){
                SetFault(ctx, FAULT_VOLTAGE_OVER);
            }
            if(ctx->cell_states[i].voltages[j] < GetSetting(ctx, MIN_GROUP_VOLTAGE)){
                SetFault(ctx, FAULT_VOLTAGE_UNDER);
            }
        }
    }
    
    if(ctx->isense.voltage1_mv > GetSetting(ctx, MAX_PACK_VOLTAGE)){
        SetFault(ctx, FAULT_PACK_VOLTAGE_OVER);
    }
    if(ctx->isense.voltage1_mv < GetSetting(ctx, MIN_PACK_VOLTAGE)){
        SetFault(ctx, FAULT_PACK_VOLTAGE_UNDER);
    }
}

void CheckTemperatureFaults(DbmsCtx* ctx){
    for(int i = 0; i < N_SIDES; ++i){
        for(int j = 0; j < N_TEMPS_PER_SIDE; ++j){
            if(ctx->cell_states[i].temps[j] > GetSetting(ctx, MAX_THERMISTOR_TEMP)){
                SetFault(ctx, FAULT_TEMP_OVER);
            }
        }
    }
}

void CheckCurrentFaults(DbmsCtx* ctx){
    if(ctx->isense.current_ma > GetSetting(ctx, MAX_CURRENT)){
        SetFault(ctx, FAULT_CURRENT_OVER);
    }
}

void CheckAllFaults(DbmsCtx* ctx){
    CheckVoltageFaults(ctx);
    CheckTemperatureFaults(ctx);
    CheckCurrentFaults(ctx);
}
