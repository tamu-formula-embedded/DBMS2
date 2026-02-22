/** 
 * 
 * Distributed BMS      Precharge Control 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */

#include "precharge.h"

void PrechargeSet(DbmsCtx* ctx)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, ctx->precharged);
}

void PrechargeUpdate(DbmsCtx* ctx)
{
  int ivt_mv = ctx->current_sensor.voltage1_mv;
  int pack_mv = ctx->stats.pack_v * 1000;
  int threshold = GetSetting(ctx, ctx->precharged ? PRECHARGE_OFF_TH : PRECHARGE_ON_TH);

  ctx->precharged = ivt_mv > pack_mv * threshold / 100 && !ctx->stats.fault_line_faulted;

  PrechargeSet(ctx);
}