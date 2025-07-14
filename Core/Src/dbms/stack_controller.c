//  
//  Copyright (c) Texas A&M University.
//  
#include "stack_controller.h"


static uint8_t FRAME_WAKE_STACK[] = { 0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95 };
static uint8_t FRAME_SHUTDOWN_STACK[] = { 0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00 };

/**
 * Provides a blocking delay in microseconds
 */
void DelayUs(DbmsCtx* ctx, uint16_t us)
{
#ifdef SIM
    usleep(us);
#else
    __HAL_TIM_SET_COUNTER(ctx->hw.timer, 0);
    uint64_t big_ctr;
    while ((big_ctr = __HAL_TIM_GET_COUNTER(ctx->hw.timer)) < us);
#endif
}

/**
 * Prints the contents of a received RxStackFrame for debugging
 */
void __PrintStackRxFrame(RxStackFrame* f)
{
    printf("RxStackFrame:\n\tinit=%d dev_addr=%d reg_addr=%d size=%d\n\tdata=",
        f->init_field, f->dev_addr, f->reg_addr, f->size);
    for (size_t i = 0; i < f->size; i++)
    {
        printf("%02x ", f->data[i]);
    }
    printf("\n");
}

/**
 * Sends a raw data frame to the battery stack via UART
 */
int SendStackFrame(DbmsCtx* ctx, uint8_t* buf, size_t len)
{
    return HAL_UART_Transmit(ctx->hw.uart, buf, len, STACK_SEND_TIMEOUT);
}


/**
 * Sends a data frame to the battery stack via UART with a CRC appended to the end for verification
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
 * Receives a data frame from the battery stack via UART
 * Job is to identify who it's from and what it's about
 */
int RecvStackFrame(DbmsCtx* ctx, RxStackFrame* rx_frame)
{
    int status = 0;
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_frame->buffer, rx_frame->size + 6, STACK_RECV_TIMEOUT)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }
    rx_frame->init_field = rx_frame->buffer[0];
    rx_frame->dev_addr = rx_frame->buffer[1];
    rx_frame->reg_addr = (rx_frame->buffer[2] << 8) + rx_frame->buffer[3];
    rx_frame->data = rx_frame->buffer + 4;
    return status;
}

/**
 * Utility function to set the baud rate of the UART
 * Used for shutdown and wakeup
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
 * Sends a wake blip to the battery stack
 */
int SendStackWakeBlip(DbmsCtx* ctx)
{
    int status = 0;
    // uart_set_brr(APBxCLK / (1 / (2.75 / (1 * 1000))));

    // 1. Switch to a very slow baud rate. receiving chips will see this
    //    as a weird, long and continuous low signal
    SetBrr(25700/2); // ?

    // 2. Send a single byte at this slow speed
    uint8_t wake_frame[] = {0x00};
    if ((status = SendStackFrame(ctx, wake_frame, sizeof(wake_frame))) != 0) 
    {
        CAN_REPORT_FAULT(ctx, status);
        return status;
    }
    // 3. Immediately switch back to the normal high speed baud rate
    //    so we can tell the chips they are awake now
    SetBrr(APBxCLK / 1000000); // 84
    DelayUs(ctx, 3500);
    return status;
}

/**
 * Sends a shutdown blip to the battery stack
 * Follows the exact same logic as the wake blip function,
 * except we are setting a different baud rate. the rate
 * is like a secret code we send to the chips to say we are waking up or shutting down
 */
int SendStackShutdownBlip(DbmsCtx* ctx)
{
    int status = 0;
    // uart_set_brr(APBxCLK / (1 / (2.75 / (1 * 1000))));
    SetBrr(128000);
    uint8_t wake_frame[] = {0x00};
    if ((status = SendStackFrame(ctx, wake_frame, sizeof(wake_frame))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
        return status;
    }
    SetBrr(APBxCLK / 1000000); // 84
    DelayUs(ctx, 3500);
    return status;
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
        HAL_Delay(15 + 12 * N_STACKDEVS);   // wtf  -- microseconds dumbass
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
    // uint8_t frame_otp_ecc_datain[] = { 0xB0, 0x03, 0x43, 0x00, 0x00, 0x00 };
    uint8_t frame_otp_ecc_datain[] = { 0xD0, 0x03, 0x4C, 0x00, 0x00, 0x00 };
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
    uint8_t frame_addr_dev[] = { 0xD0, 0x03, 0x06, 0x00, 0x00, 0x00 };
    for (int i = 0; i <= N_STACKDEVS; i++) {
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
    uint8_t frame_set_stack_base[] = { 0x90, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00 }; 
    uint8_t frame_set_stack_top[] = { 0x90, N_STACKDEVS-1, 0x03, 0x08, 0x03, 0x00, 0x00 }; 
    SendStackFrameSetCrc(ctx, frame_set_stack_base, sizeof(frame_set_stack_base));
    SendStackFrameSetCrc(ctx, frame_set_stack_top, sizeof(frame_set_stack_top));
}

/**
 * Reads the OTP (one-time programmable) ECC (error correction code)
 * Used to initalize the stack
 */
void ReadOtpEccDatain(DbmsCtx* ctx)  
{
    // uint8_t frame_otp_ecc_datain[] = { 0xA0, 0x03, 0x43, 0x00, 0x00, 0x00};
    uint8_t frame_otp_ecc_datain[] = { 0xC0, 0x03, 0x4C, 0x00, 0x00, 0x00};
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(ctx, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}

/**
 * "main" function for initalization
 * Builds a fully configured and operational battery stack
 * Uses helper functions defined above
 */
void StackAutoAddr(DbmsCtx* ctx)
{
    SendOtpEccDatain(ctx); // step 1

    static uint8_t FRAME_ENABLE_AUTO_ADDR[] = { 0xD0, 0x03, 0x09, 0x01, 0x0F, 0x74 };
    SendStackFrameSetCrc(ctx, FRAME_ENABLE_AUTO_ADDR, sizeof(FRAME_ENABLE_AUTO_ADDR)); // step 2

    SendAutoAddr(ctx); // step 3

    static uint8_t FRAME_SET_ALL_STACK[] = { 0xD0, 0x03, 0x08, 0x02, 0x0, 0x0 };
    SendStackFrameSetCrc(ctx, FRAME_SET_ALL_STACK, sizeof(FRAME_SET_ALL_STACK)); // step 4

    SendSetStackTop(ctx); // step 5

    ReadOtpEccDatain(ctx); // step 6

    static uint8_t FRAME_READ_DIR0_ADDR[] = { 0xA0, 0x00, 0x03, 0x06, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_READ_DIR0_ADDR, sizeof(FRAME_READ_DIR0_ADDR));

    static uint8_t FRAME_CONF1_READ[] = { 0x80, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_CONF1_READ, sizeof(FRAME_CONF1_READ));
}

/**
 * Sets the number of active cells in the stack
 * Used to initalize the stack
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells)
{
    // possible sw guide error 0xB0 (stack write) --> 0xD0 (broadcast write)
    uint8_t frame_set_active_cell[] =  { 0xD0, 0x00, N_STACKDEVS, n_active_cells, 0x00, 0x00 };

    SendStackFrameSetCrc(ctx, frame_set_active_cell, sizeof(frame_set_active_cell));
}

void StackSetupGpio(DbmsCtx* ctx)
{
    // TODO: implement
}

void StackSetupTempReadings(DbmsCtx* ctx)
{

}

/**
 * Tells all battery monitoring chips to start measuring the voltage of each cell
 */
void StackSetupVoltReadings(DbmsCtx* ctx)
{
    uint8_t FRAME_SET_ADC_CONT_RUN[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00 };
    SendStackFrameSetCrc(ctx, FRAME_SET_ADC_CONT_RUN, sizeof(FRAME_SET_ADC_CONT_RUN));
}

/**
 * Receives the voltage readings from the battery stack
 */
void StackUpdateVoltReadings(DbmsCtx* ctx)
{
    // Step 1: ask all chips to send their voltage readings
    uint8_t FRAME_ADC_MEASUREMENTS[] = { 0xC0, 0x05, 0x68 + 2*(16-N_GROUPS), N_GROUPS*2-1, 0x00, 0x00 };
    SendStackFrameSetCrc(ctx, FRAME_ADC_MEASUREMENTS, sizeof(FRAME_ADC_MEASUREMENTS));
    DelayUs(ctx, 192 + 5 * N_STACKDEVS);

    // Step 2: after waiting for their voltage readings, prepare to receive data
    uint8_t addr;
    STACK_DEFINE_RX_FRAME(rx_frame, N_GROUPS * sizeof(int16_t))

    // Step 3: loop through each chip and process its response
    for (size_t i = 0; i < N_STACKDEVS; i++)
    {
        
        RecvStackFrame(ctx, &rx_frame);

        // Step 4: send the voltage readings to the CAN bus
        uint8_t frame[8] = {0};
        memcpy(frame, rx_frame.buffer, 8);
        CanTransmit(ctx, 0x581, rx_frame.buffer);

        
        if ((addr = rx_frame.dev_addr - 1) < 0) continue;  // skip myself
        // for (size_t j = 0; j < N_GROUPS; j++)
        // {
        //     ctx->cell_states[addr / N_MONITORS_PER_SEG][addr % N_MONITORS_PER_SEG].voltages[j]
        //         = (rx_frame.data[j * sizeof(int16_t)] << 8) 
        //         + (rx_frame.data[j * sizeof(int16_t) + 1]);     // watch out! plus 1 inside
        // }
    }
}
