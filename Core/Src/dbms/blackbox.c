#include "blackbox.h"
#include "context.h"
#include "storage.h"
#include "vehicle_interface.h"
#include "common.h"

void BlackboxInit(DbmsCtx* ctx)
{
    memset(ctx->blackbox.old_data, 0, sizeof(Snapshot));
    memset(ctx->blackbox.new_data, 0, sizeof(Snapshot));
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
    blackbox->iter = 100;
}

int SendIndividualBlackbox(DbmsCtx* ctx, bool old)
{
    int status = 0;
    Snapshot* blackbox = old ? GetBlackboxOld(ctx) : GetBlackboxNew(ctx);
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

int SaveBlackboxToEEPROM(DbmsCtx* ctx, Snapshot* old_blackbox, Snapshot* new_blackbox)
{
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

Snapshot* GetBlackboxOld(DbmsCtx* ctx)
{
    return ctx->blackbox.old_data;
}

Snapshot* GetBlackboxNew(DbmsCtx* ctx)
{
    return ctx->blackbox.new_data;
}
