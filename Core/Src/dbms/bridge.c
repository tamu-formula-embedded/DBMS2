/**
 * Distributed BMS Bridge Module
 * 
 * @file bridge.c
 * @copyright (C) 2025 Texas A&M University
 * @author Justus Languell <justus@tamu.edu>
 * 
 * @brief Low level interface with the bridge chip,
 * and then to the stack, over UART.
 */
#include "bridge.h"

/**
 * @brief Sends a raw data frame to the battery stack via UART
 * 
 * @param ctx Context pointer
 * @param buf The data for the frame to send
 * @param len The length of the frame data
 * @return Error code
 */
int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len)
{
    return HAL_UART_Transmit(ctx->hw.uart, buf, len, STACK_SEND_TIMEOUT);
}

/**
 * @brief Sends a data frame to the stack via UART with
 * a CRC appended to the end for verification
 * 
 * @param ctx Context pointer
 * @param buf The data for the frame to send
 * @param len The length of the frame data
 * @return Error code
 */
int SendStackFrameSetCrc(DbmsCtx* ctx, uint8_t* buf, size_t len)
{
    int status = 0;
    // Calculate the CRC on the payload
    uint16_t crc = CalcCrc16(buf, len - 2);
    // Tape the CRC to the end of the frame
    buf[len - 2] = crc & 0xFF;
    buf[len - 1] = (crc >> 8) & 0xFF;
    // Send the frame
    status = HAL_UART_Transmit(ctx->hw.uart, buf, len, STACK_SEND_TIMEOUT);
    // Clear the CRC from the payload
    buf[len - 2] = 0;
    buf[len - 1] = 0;
    return status;
}

/**
 * @brief Utility function to set the baud rate of the UART
 * Used for shutdown and wakeup
 * 
 * @param brr Some timing property
 */
void SetBrr(uint64_t brr)
{
#ifndef SIM
    UART4->CR1 &= ~(USART_CR1_UE);
    UART4->BRR = brr;
    UART4->CR1 |= USART_CR1_UE;
#endif
}

/**
 * @brief Send a "blip", where we pull the line low for a little bit.
 * Right now the length of the blip is set by BRR.
 *
 * @todo Make it accept param as us or something more intuitive.
 * 
 * @param ctx Context pointer
 * @param brr Some timing poperty
 * @return Error code
 */
int SendStackBlip(DbmsCtx* ctx, uint64_t brr)
{
    int status = 0;

    // Switch to a very slow baud rate. receiving chips will see this
    // as a continuous low signal
    SetBrr(brr); 

    // Send a single byte at this slow speed
    uint8_t wake_frame[] = {0x00};
    if ((status = SendStackFrame(ctx, wake_frame, sizeof(wake_frame))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        return status;
    }

    // Switch back to the normal high speed baud rate
    SetBrr(APBxCLK / 1000000); // 84
    DelayUs(ctx, 3500);
    return status;
}