/**
 *
 * Distributed BMS      Vehicle Interface
 *
 * Copyright (C) 2025   Texas A&M University
 *
 *                      Eli Nicksic <eli.n@tamu.edu>
 */

#ifndef _OBJSTORE_H_
#define _OBJSTORE_H_

#include "context.h"
#include "utils/common.h"
#include "storage.h"

#define OBJSTORE_MAX_ID (EEPROM_PAGE_SIZE / 4 - 1)

int ObjStoreSave(DbmsCtx *ctx, uint8_t id, uint32_t value);
int ObjStoreSend(DbmsCtx *ctx, uint8_t id);

int ObjStoreIter(DbmsCtx *ctx);
int ObjStoreHandleSave(DbmsCtx *ctx, uint8_t frame[8]);
int ObjStoreHandleReq(DbmsCtx *ctx, uint8_t frame[8]);

#endif