/** 
 * 
 * Distributed BMS      Elcon Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Justus Languell  <justus@tamu.edu>
 */

#include "elcon.h"

void HandleElconHeartbeat(DbmsCtx* ctx, uint8_t* data)
{
    ctx->elcon.heartbeat = HAL_GetTick();
    CanLog(ctx, "t %d\n", ctx->elcon.heartbeat);

    ctx->elcon.voltage_out = be16_to_u16(data);
    ctx->elcon.current_out = be16_to_u16(data+2);

    CanLog(ctx, "%d\n", (int) ctx->elcon.voltage_out);
    CanLog(ctx, "%d\n", (int) ctx->elcon.current_out);

    ctx->elcon.status_flags = data[5];
}
