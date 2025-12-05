/** 
 * 
 * Distributed BMS      Stack Module -> Stack Control Sequences
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#include "stack.h"

/**
 * Sends a wake blip to the battery stack
 * 
 */
int SendStackWakeBlip(DbmsCtx* ctx)
{
    return SendStackBlip(ctx, 25700 / 2);
}
/**
 * Sends a shutdown blip to the battery stack
 */
int SendStackShutdownBlip(DbmsCtx* ctx)
{
    return SendStackBlip(ctx, 128000);
}

/**
 * Wake the battery stack
 */
int StackWake(DbmsCtx* ctx)
{
    static uint8_t FRAME_WAKE_STACK[] = {0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95};
    
    int status = 0;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackWakeBlip(ctx)) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackFrame(ctx, FRAME_WAKE_STACK, sizeof(FRAME_WAKE_STACK))) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        HAL_Delay(15 + 12 * N_STACKDEVS); // wtf  -- microseconds dumbass
    }
    return status;
}

/**
 * Shuts down the battery stack
 * Use this when we turn the vehicle off, or when going to sleep
 * We could also use this in response to a critical fault
 */
int StackShutdown(DbmsCtx* ctx)
{
    static uint8_t FRAME_SHUTDOWN_STACK[] = {0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00};

    int status = 0;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackFrameSetCrc(ctx, FRAME_SHUTDOWN_STACK, sizeof(FRAME_SHUTDOWN_STACK))) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackShutdownBlip(ctx)) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        HAL_Delay(100);
    }
    return status;
}

void SendOtpEccDatain(DbmsCtx* ctx)
{
    uint8_t frame_otp_ecc_datain[] = {0xD0, 0x03, 0x43, 0x00, 0x00, 0x00};
    // uint8_t frame_otp_ecc_datain[] = { 0xD0, 0x03, 0x4C, 0x00, 0x00, 0x00 };
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(ctx, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}

void SendAutoAddr(DbmsCtx* ctx)
{
    // 0xB0 at first
    uint8_t frame_addr_dev[] = {0xD0, 0x03, 0x06, 0x00, 0x00, 0x00};
    for (int i = 0; i <= N_STACKDEVS; i++)
    {
        SendStackFrameSetCrc(ctx, frame_addr_dev, sizeof(frame_addr_dev));
        frame_addr_dev[3]++;
    }
}

void SendSetStackTop(DbmsCtx* ctx)
{
    // Sets all devices as stack devices
    uint8_t frame_set_stack_devices[] = {0xD0, 0x03, 0x08, 0x02, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_devices, sizeof(frame_set_stack_devices));

    // Sets bridge device as non-stack device and bottom of stack
    uint8_t frame_set_stack_base[] = {0x90, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_base, sizeof(frame_set_stack_base));

    // Sets top of stack
    uint8_t frame_set_stack_top[] = {0x90, N_STACKDEVS - 1, 0x03, 0x08, 0x03, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_top, sizeof(frame_set_stack_top));
}

void ReadOtpEccDatain(DbmsCtx* ctx)
{
    // uint8_t frame_otp_ecc_datain[] = { 0xA0, 0x03, 0x43, 0x00, 0x00, 0x00};
    uint8_t frame_otp_ecc_datain[] = {0xC0, 0x03, 0x4C, 0x00, 0x00, 0x00};
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(ctx, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}


/**
 * Full auto addressing procedure
 */
void StackAutoAddr(DbmsCtx* ctx)
{
    SendOtpEccDatain(ctx); // step 1

    static uint8_t FRAME_ENABLE_AUTO_ADDR[] = {0xD0, 0x03, 0x09, 0x01, 0x0F, 0x74};
    SendStackFrameSetCrc(ctx, FRAME_ENABLE_AUTO_ADDR, sizeof(FRAME_ENABLE_AUTO_ADDR)); // step 2

    SendAutoAddr(ctx); // step 3

    static uint8_t FRAME_SET_ALL_STACK[] = {0xD0, 0x03, 0x08, 0x02, 0x0, 0x0};
    SendStackFrameSetCrc(ctx, FRAME_SET_ALL_STACK, sizeof(FRAME_SET_ALL_STACK)); // step 4

    SendSetStackTop(ctx); // step 5

    ReadOtpEccDatain(ctx); // step 6

    // static uint8_t FRAME_READ_DIR0_ADDR[] = { 0xA0, 0x00, 0x03, 0x06, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_READ_DIR0_ADDR, sizeof(FRAME_READ_DIR0_ADDR));

    // static uint8_t FRAME_CONF1_READ[] = { 0x80, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_CONF1_READ, sizeof(FRAME_CONF1_READ));
}

/**
 * Set the number of active cells in the stack
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells)
{
    uint8_t frame_set_active_cell[] = {0xD0, 0x00, 0x03, n_active_cells, 0x00, 0x00};

    SendStackFrameSetCrc(ctx, frame_set_active_cell, sizeof(frame_set_active_cell));
}