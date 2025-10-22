/** 
 * 
 * Distributed BMS      Stack Module -> Stack LED control
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#include "stack.h"

int ToggleMonitorChipLed(DbmsCtx* ctx, bool on, uint8_t dev_number)
{
    // Turns on or off LED connected to GPIO8 on the specified monitor chip
    int status = 0;
    uint8_t on_off_value = 0x29; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x21;
    uint8_t led_change_write_com[] = {0x90, dev_number, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send single device write command frame
    if ((status = SendStackFrameSetCrc(ctx, led_change_write_com, sizeof(led_change_write_com))) != 0)
    {
        return status;
    }
    return status;
}

int ToggleAllMonitorChipLeds(DbmsCtx* ctx, bool on)
{
    // Turns on or off LED connected to GPIO8 on all monitor chips
    // Note for future:
    int status = 0;
    uint8_t on_off_value = 0x29; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x21;
    uint8_t leds_change_write_com[] = {0xB0, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send stack device write command frame
    if ((status = SendStackFrameSetCrc(ctx, leds_change_write_com, sizeof(leds_change_write_com))) != 0)
    {
        return status;
    }
    return status;
}

void MonitorLedBlink(DbmsCtx* ctx)
{

    uint64_t curr_ts = GetUs(ctx) - ctx->m_led_blink_ts;

    if (ctx->m_led_on)
    {
        if (curr_ts > 100000)
        {
            ToggleAllMonitorChipLeds(ctx, false);
            ctx->m_led_on = false;
            ctx->m_led_blink_ts = GetUs(ctx);
        }
    }
    else
    {
        if (curr_ts > 1000000)
        {
            ToggleAllMonitorChipLeds(ctx, true);
            ctx->m_led_on = true;
            ctx->m_led_blink_ts = GetUs(ctx);
        }
    }
}