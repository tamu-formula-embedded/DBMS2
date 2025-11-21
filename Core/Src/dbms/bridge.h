/**
 * Distributed BMS Bridge Module
 * 
 * @file bridge.h
 * @copyright (C) 2025 Texas A&M University
 * @author Justus Languell <justus@tamu.edu>
 * 
 * @brief Low level interface with the bridge chip,
 * and then to the stack, over UART.
 */
#ifndef _BRIDGE_H_
#define _BRIDGE_H_

#include "utils/common.h"

#include "faults/faults.h"
#include "context.h"
#include "model/data.h"

#include "can.h"
#include "vinterface.h"

// TODO: optimize the shit out of this
#define STACK_SEND_TIMEOUT 2
#define STACK_RECV_TIMEOUT 2

#define APBxCLK 42000000    // TODO: fix legacy name

#define RX_FRAME_SIZE(SIZE) (SIZE+6)

/**
 * @brief Sends a raw data frame to the battery stack via UART
 * 
 * @param ctx Context pointer
 * @param buf The data for the frame to send
 * @param len The length of the frame data
 * @return Error code
 */
int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len);

/**
 * @brief Sends a data frame to the stack via UART with
 * a CRC appended to the end for verification
 * 
 * @param ctx Context pointer
 * @param buf The data for the frame to send
 * @param len The length of the frame data
 * @return Error code
 */
int SendStackFrameSetCrc(DbmsCtx* ctx, uint8_t* buf, size_t len);

/**
 * @brief Utility function to set the baud rate of the UART
 * Used for shutdown and wakeup
 * 
 * @param brr Some timing property
 */
void SetBrr(uint64_t brr);

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
int SendStackBlip(DbmsCtx* ctx, uint64_t brr);

#endif