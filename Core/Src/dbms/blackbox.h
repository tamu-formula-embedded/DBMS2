/** 
 * 
 * Distributed BMS      Blackbox (Snapshot Saving Utility)
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _BLACKBOX_H_
#define _BLACKBOX_H_

#include "context.h"
#include "faults/faults.h"

void BlackboxInit(DbmsCtx* ctx);

void BlackboxUpdate(DbmsCtx* ctx);

void PopulateBlackboxInfo(DbmsCtx* ctx, Snapshot* blackbox);

int BlackboxSend(DbmsCtx* ctx);

int BlackboxSaveOnFault(DbmsCtx* ctx);


#endif