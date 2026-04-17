/** 
 * 
 * Distributed BMS      Elcon Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#ifndef _ELCON_H_
#define _ELCON_H_

#include "can.h"
#include "vinterface.h"

void HandleElconHeartbeat(DbmsCtx* ctx, uint8_t* data);

void SendElconRequest(DbmsCtx* ctx, int16_t v_req, int16_t i_req, bool en);

#endif