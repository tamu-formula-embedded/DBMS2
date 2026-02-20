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

void PrechargeSet(DbmsCtx *ctx, bool enabled) {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, enabled);
  ctx->precharged = enabled;
}

void PrechargeUpdate(DbmsCtx *ctx) {
  float threshold = GetSetting(ctx, PRECHARGE_TH) / 100.0;
  // PrechargeSet(ctx, (ctx->current_sensor.voltage1_mv >= threshold * ctx->stats.pack_v));
  PrechargeSet(ctx, false);
}