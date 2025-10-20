/*
 * 
 * Distributed BMS      CAN Interface 
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell <justus@tamu.edu>
 */

#ifndef _CAN_H_
#define _CAN_H_

typedef enum 
{
    CAN_RX_0, 
    CAN_RX_1,
} CanRxChannel;

typedef struct 
{

} CanCtx;


int ConfigCAN(CanCtx* ctx);

#endif
