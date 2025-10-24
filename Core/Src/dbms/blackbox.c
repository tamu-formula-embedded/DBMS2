/** 
 * 
 * Distributed BMS      Blackbox (Snapshot Saving Utility)
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Cam Stone <cameron28202@tamu.edu>
 */
#include "blackbox.h"
#include "context.h"
#include "storage.h"
#include "can.h"
#include "utils/common.h"

static Snapshot snapshot_storage[2];

void BlackboxInit(DbmsCtx* ctx)
{
    memset(snapshot_storage, 0, sizeof(snapshot_storage));
    
    ctx->blackbox.old_data = &snapshot_storage[0];
    ctx->blackbox.new_data = &snapshot_storage[1];
}

void BlackboxSwapAndUpdate(DbmsCtx* ctx)
{
    Snapshot* temp = ctx->blackbox.old_data;
    ctx->blackbox.old_data = ctx->blackbox.new_data;
    ctx->blackbox.new_data = temp;
    
    PopulateBlackboxInfo(ctx, ctx->blackbox.new_data);
}

void PopulateBlackboxInfo(DbmsCtx* ctx, Snapshot* blackbox)
{
    {
        memset(blackbox, 0, sizeof(Snapshot));
        
        // Iteration counter
        blackbox->iter = ctx->stats.iters;
        
        // Current sensor data
        blackbox->current_ma = ctx->isense.current_ma;
        blackbox->voltage_mv = ctx->isense.voltage1_mv;
        
        // Charge stats
        blackbox->qd = ctx->qstats.accumulated_loss;
        
        // Current limits and resistance
        blackbox->current_limit_a = ctx->model.I_lim;
        blackbox->dcir = ctx->model.R_cell;
        blackbox->total_resistance = ctx->model.R0 + ctx->model.R1 + ctx->model.R2;
        
        // Temperature stats
        blackbox->avg_temp = ctx->stats.avg_t;
        blackbox->max_temp = ctx->stats.max_t;
        blackbox->min_temp = ctx->stats.min_t;
        
        // Cell voltage delta
        blackbox->cell_delta_v = ctx->stats.max_v - ctx->stats.min_v;
        
        // Voltage stats
        blackbox->high_voltage = ctx->stats.max_v;
        blackbox->low_voltage = ctx->stats.min_v;
        blackbox->avg_voltage = ctx->stats.avg_v;
        
        // Faults
        blackbox->controller_mask = ctx->faults.controller_mask;
        blackbox->bridge_fault_summary = ctx->faults.bridge_fault_summary;
        blackbox->bridge_faults = ctx->faults.bridge_faults;
        memcpy(blackbox->monitor_fault_summary, ctx->faults.monitor_fault_summary, N_MONITORS);
    }
}

int SendIndividualBlackbox(DbmsCtx* ctx, bool old)
{
    int status = 0;
    Snapshot temp_snapshot;
    
    // load from EEPROM into temporary buffer
    status = LoadStoredObject(ctx, old ? EEPROM_BLACKBOX_OLD_ADDR : EEPROM_BLACKBOX_NEW_ADDR, 
                             &temp_snapshot, sizeof(Snapshot));
    
    if(status != HAL_OK){
        //todo
    }
    
    uint8_t* blackbox_ptr = (uint8_t*)&temp_snapshot;

    for(uint16_t i = 0; i < sizeof(Snapshot); i += 6)
    {
        uint8_t frame[8] = {0};
        
        // first 2 bytes are the frame number
        frame[0] = (i / 6) & 0xFF;
        frame[1] = (i / 6) >> 8;

        // copy next 6 bytes
        for(uint8_t j = 0; j < 6; j++)
        {
            frame[j + 2] = blackbox_ptr[i + j];
        }

        status = CanTransmit(ctx, old ? CANID_BLACKBOX_OLD : CANID_BLACKBOX_NEW, frame);
        if(status != HAL_OK)
        {
            return status;
        }
        HAL_Delay(1);
    }
    return status;
}

int BlackboxSend(DbmsCtx* ctx)
{
    int status = 0;

    status = SendIndividualBlackbox(ctx, true);
    if (status != HAL_OK)
    {
        return status;
    }
    status = SendIndividualBlackbox(ctx, false);
    if (status != HAL_OK)
    {
        return status;
    }
    return status;
}

int BlackboxSaveOnFault(DbmsCtx* ctx, Snapshot* old_blackbox, Snapshot* new_blackbox)
{
    // save to eeprom
    int status = 0;
    status = SaveStoredObject(ctx, EEPROM_BLACKBOX_OLD_ADDR, old_blackbox, sizeof(Snapshot));
    if (status != HAL_OK)
    {
        return status;
    }
    status = SaveStoredObject(ctx, EEPROM_BLACKBOX_NEW_ADDR, new_blackbox, sizeof(Snapshot));
    if (status != HAL_OK)
    {
        return status;
    }
    return status;
}