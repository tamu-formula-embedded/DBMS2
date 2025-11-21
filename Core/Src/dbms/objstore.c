#include "objstore.h"

#define OBJ_ADDR(id) (EEPROM_OBJSTORE_ADDR + (id) * sizeof(uint32_t))

int ObjStoreSave(DbmsCtx* ctx, uint8_t id, uint32_t value) 
{
    if (id > OBJSTORE_MAX_ID) return -1;
    int status = 0;
    
    status = SaveStoredObject(ctx, OBJ_ADDR(id), &value, sizeof(value));

    uint8_t frame[8] = {0};
    frame[0] = id;
    frame[1] = status ? 1 : 0;

    frame[4] = (value >> 24) & 0xFF;
    frame[5] = (value >> 16) & 0xFF;
    frame[6] = (value >> 8) & 0xFF;
    frame[7] = value & 0xFF;

    status = CanTransmit(ctx, CANID_TX_OBJSTORE_ACK, frame);
    return status;
}

int ObjStoreSend(DbmsCtx* ctx, uint8_t id) 
{
    if (id > OBJSTORE_MAX_ID) return -1;
    int status = 0;

    uint32_t value;
    status = LoadStoredObject(ctx, OBJ_ADDR(id), &value, sizeof(value));
    if (status != HAL_OK) return status;

    uint8_t frame[8] = {0};
    frame[0] = id;
    frame[4] = (value >> 24) & 0xFF;
    frame[5] = (value >> 16) & 0xFF;
    frame[6] = (value >> 8) & 0xFF;
    frame[7] = value & 0xFF;

    status = CanTransmit(ctx, CANID_TX_OBJSTORE_SEND, frame);
    return status;
}

int ObjStoreIter(DbmsCtx* ctx)
{
    int status = 0;
    switch (ctx->objstore.req)
    {
    case OBJSTORE_REQ_READ:
        status = ObjStoreSend(ctx, ctx->objstore.req_id);
        break;
    case OBJSTORE_REQ_WRITE:
        status = ObjStoreSave(ctx, ctx->objstore.req_id, ctx->objstore.req_val);
        break;
    default:
        break;
    }

    return status;
}

int ObjStoreHandleSave(DbmsCtx* ctx, uint8_t frame[8]) {
    if (ctx->objstore.req) return HAL_BUSY;

    uint8_t id = frame[0];
    uint32_t value = 0;
    value |= ((uint32_t)frame[4]) << 24;
    value |= ((uint32_t)frame[5]) << 16;
    value |= ((uint32_t)frame[6]) << 8;
    value |= ((uint32_t)frame[7]);

    ctx->objstore.req = OBJSTORE_REQ_WRITE;
    ctx->objstore.req_id = id;
    ctx->objstore.req_val = value;

    return HAL_OK;
}

int ObjStoreHandleReq(DbmsCtx* ctx, uint8_t frame[8]) {
    if (ctx->objstore.req) return HAL_BUSY;

    uint8_t id = frame[0];

    ctx->objstore.req = OBJSTORE_REQ_READ;
    ctx->objstore.req_id = id;
    ctx->objstore.req_val = 0;

    return HAL_OK;
}
