//
//  Copyright (c) Texas A&M University.
//
#ifndef _H_SCHED_H
#define _H_SCHED_H

#include "common.h"
#include "context.h"
#include "storage.h"

#ifdef SIM
#include "time.h"
#endif

//
//  Timer and scheduler related functions and objects
//

/**
 * Blocking delay in microseconds using the free-running timer.
 */
void DelayUs(DbmsCtx* ctx, uint32_t us);

/**
 * get time in microseconds from the hardware timer (monotonic, 64-bit)
 * Assumes timer tick = 1 us. Works with 16- or 32-bit counters.
 */
uint64_t GetUs(DbmsCtx* ctx);

/**
 * Calculate how many microseconds to wait so each loop iteration lasts ~1/target_hz seconds.
 * Uses iter_start_us and iter_end_us captured around your work.
 * Returns 0 if you’re already late.
 */
uint32_t CalcIterDelay(DbmsCtx* ctx, uint32_t hz);

#endif