#include "stack.h"

// static uint8_t FRAME_WAKE_STACK[] = {0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95};
// static uint8_t FRAME_SHUTDOWN_STACK[] = {0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00};

/**
 * Sends a wake blip to the battery stack
 * 
 */
int SendStackWakeBlip(DbmsCtx* ctx)
{
    return SendStackBlip(25700 / 2);
}
/**
 * Sends a shutdown blip to the battery stack
 */
int SendStackShutdownBlip(DbmsCtx* ctx)
{
    return SendStackBlip(128000);
}

/**
 * Wakes up the battery stack
 * Use this when we turn the vehicle on, or when waking up from a sleep mode
 * We could also use this to recover from a communication error (if we lose contact with the stack for some reason)
 */
int StackWake(DbmsCtx* ctx)
{
    int status = HAL_OK;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackWakeBlip(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackFrame(ctx, FRAME_WAKE_STACK, sizeof(FRAME_WAKE_STACK))) != HAL_OK)
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
    int status = HAL_OK;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackFrameSetCrc(ctx, FRAME_SHUTDOWN_STACK, sizeof(FRAME_SHUTDOWN_STACK))) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackShutdownBlip(ctx)) != HAL_OK)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        HAL_Delay(100);
    }
    return status;
}

/**
 * Sends the OTP (one-time programmable) ECC (error correction code)
 * Used to initalize the stack
 */
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

/**
 * Automatically assigns a unique address to each battery monitoring chip in the stack
 * For example, if we want to ask what the voltage of chip 5 is, we need to make one
 * of them chip #5, another chip #6, and so on
 * Used to initalize the stack
 */
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

/**
 * Tells the chips who is at the bottom and top of the stack
 * Used to initalize the stack
 */
void SendSetStackTop(DbmsCtx* ctx)
{
    // 2nd byte replaced with id (num of stack devs), last 2 bytes for crc

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

/**
 * Reads the OTP (one-time programmable) ECC (error correction code)
 * Used to initalize the stack
 */
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