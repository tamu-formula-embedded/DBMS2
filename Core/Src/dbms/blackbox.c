#include "blackbox.h"
#include "context.h"
#include "storage.h"
#include "vehicle_interface.h"
#include "common.h"


static Snapshot old_snapshot_storage;
static Snapshot new_snapshot_storage;
static Snapshot saved_old_snapshot_storage;
static Snapshot saved_new_snapshot_storage;

void BlackboxInit(DbmsCtx* ctx)
{
    ctx->blackbox.old_data = &old_snapshot_storage;
    ctx->blackbox.new_data = &new_snapshot_storage;
    ctx->blackbox.saved_old_data = &saved_old_snapshot_storage;
    ctx->blackbox.saved_new_data = &saved_new_snapshot_storage;
    
    memset(ctx->blackbox.old_data, 0, sizeof(Snapshot));
    memset(ctx->blackbox.new_data, 0, sizeof(Snapshot));
    memset(ctx->blackbox.saved_old_data, 0, sizeof(Snapshot));
    memset(ctx->blackbox.saved_new_data, 0, sizeof(Snapshot));
    
    // // Try to load saved snapshots from EEPROM
    // int status = LoadStoredObject(ctx, EEPROM_BLACKBOX_OLD_ADDR, 
    //                                ctx->blackbox.saved_old_data, sizeof(Snapshot));
    // if (status == HAL_OK) {
    //     status = LoadStoredObject(ctx, EEPROM_BLACKBOX_NEW_ADDR, 
    //                               ctx->blackbox.saved_new_data, sizeof(Snapshot));
    // }
    
    // // If load failed or no data, zero out the saved snapshots
    // if (status != HAL_OK) {
    //     memset(ctx->blackbox.saved_old_data, 0, sizeof(Snapshot));
    //     memset(ctx->blackbox.saved_new_data, 0, sizeof(Snapshot));
    // }
}

void BlackboxSwapAndUpdate(DbmsCtx* ctx)
{
    // swap the pointers
    Snapshot* temp = ctx->blackbox.old_data;
    ctx->blackbox.old_data = ctx->blackbox.new_data;
    ctx->blackbox.new_data = temp;
    
    // populate the new blackbox with current state
    PopulateBlackboxInfo(ctx, ctx->blackbox.new_data);
}

void PopulateBlackboxInfo(DbmsCtx* ctx, Snapshot* blackbox)
{
    // this happens every iteration - update all important info
    // blackbox->important = ctx->important
    memset(blackbox, 0, sizeof(Snapshot));
    
    blackbox->iter = ctx->stats.iters;
}

int SendIndividualBlackbox(DbmsCtx* ctx, bool old)
{
    int status = 0;
    Snapshot* blackbox = old ? GetBlackboxSavedOld(ctx) : GetBlackboxSavedNew(ctx);
    uint8_t* blackbox_ptr = (uint8_t*)blackbox;

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

void BlackboxSaveOnFault(DbmsCtx* ctx)
{
    // save the current blackbox to the saved blackbox
    memcpy(ctx->blackbox.saved_old_data, ctx->blackbox.old_data, sizeof(Snapshot));
    memcpy(ctx->blackbox.saved_new_data, ctx->blackbox.new_data, sizeof(Snapshot));
}

int SaveBlackboxToEEPROM(DbmsCtx* ctx, Snapshot* old_blackbox, Snapshot* new_blackbox)
{
    // copy values
    BlackboxSaveOnFault(ctx);

    // save to eeprom
    int status = 0;
    status = SaveStoredObject(ctx, EEPROM_BLACKBOX_OLD_ADDR, old_blackbox, sizeof(Snapshot));
    if (status != HAL_OK)
    {
        return status;
    }
    HAL_Delay(8);
    status = SaveStoredObject(ctx, EEPROM_BLACKBOX_NEW_ADDR, new_blackbox, sizeof(Snapshot));
    if (status != HAL_OK)
    {
        return status;
    }
    return status;
}

Snapshot* GetBlackboxOld(DbmsCtx* ctx)
{
    return ctx->blackbox.old_data;
}

Snapshot* GetBlackboxNew(DbmsCtx* ctx)
{
    return ctx->blackbox.new_data;
}

Snapshot* GetBlackboxSavedOld(DbmsCtx* ctx)
{
    return ctx->blackbox.saved_old_data;
}

Snapshot* GetBlackboxSavedNew(DbmsCtx* ctx)
{
    return ctx->blackbox.saved_new_data;
}
