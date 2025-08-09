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
    static uint8_t FRAME_SET_ADC_CONT_RUN[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00 };
    SendStackFrameSetCrc(ctx, FRAME_SET_ADC_CONT_RUN, sizeof(FRAME_SET_ADC_CONT_RUN));
}

int FillStackFrame(RxStackFrame* rx_frame, uint8_t* buffer, size_t size)
{
    int status = 0;
    rx_frame->init_field = buffer[0];
    rx_frame->dev_addr = buffer[1];
    rx_frame->reg_addr = (buffer[2] << 8) + buffer[3];
    rx_frame->data = buffer + 4;
    rx_frame->size = size;
    rx_frame->crc = (buffer[4+size] << 8) + buffer[4+size+1];
    if (rx_frame->crc == CalcCrc16(rx_frame->data, rx_frame->size))
    {
        // todo: do something here?
    }
    return status;
}

int FillStackFrames(RxStackFrame* rx_frames, uint8_t* buffer, size_t size, size_t n_frames)
{
    for (size_t i = 0; i < n_frames; i++)
    {
        FillStackFrame(rx_frames + i, buffer + (i * RX_FRAME_SIZE(size)), n_frames);
    }
    return 0;
}

void StackUpdateVoltReadings(DbmsCtx* ctx)
{
    static uint8_t rx_buffer_volt_readings[1024];  

    int status = 0;
    uint8_t FRAME_ADC_MEASUREMENTS[] = { 0xC0, 0x05, 0x68 + 2*(16-N_GROUPS), N_GROUPS*2-1, 0x00, 0x00 };

    if ((status = SendStackFrameSetCrc(ctx, FRAME_ADC_MEASUREMENTS, sizeof(FRAME_ADC_MEASUREMENTS))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }
    // DelayUs(ctx, 192 + 5 * N_STACKDEVS); // no !

    size_t data_size = N_GROUPS * sizeof(int16_t);

    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_STACKDEVS; // <- num frames
    //                 header+crc ^         ^ readings * 2 bytes each    

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_volt_readings, expected_rx_size, STACK_RECV_TIMEOUT)) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }

    RxStackFrame rx_frames[N_STACKDEVS];
    FillStackFrames(rx_frames, rx_buffer_volt_readings, data_size, N_STACKDEVS);

    int8_t addr;
    for (size_t i = 0; i < N_STACKDEVS; i++)
    {
        if (rx_frames[i].dev_addr == 0) 
            continue; // this is myself

        addr = rx_frames[i].dev_addr - 1;
        for (size_t j = 0; j < N_GROUPS; j++)
        {
            ctx->cell_states[addr / N_MONITORS_PER_SEG][addr % N_MONITORS_PER_SEG].voltages[j]
                = (rx_frames[i].data[j * sizeof(int16_t)] << 8) 
                + (rx_frames[i].data[j * sizeof(int16_t) + 1]);     // watch out! plus 1 inside
        } 
    }
}

int FaultOtpErrorCheck(DbmsCtx* ctx){
        
    int status = 0;
    // FAULT_OTP Reg addr = 0x0535
    uint8_t* buffer = {0xA0, 0x05, 0x35, 0x00, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, buffer, sizeof(buffer))) != 0){
        return status;
    }
    // Receiving the FAULT_OTP register data from all monitor chips
    size_t expected_rx_size = RX_FRAME_SIZE(sizeof(uint8_t)) * N_STACKDEVS;
    uint8_t* buf[expected_rx_size];
    if ((status = HAL_UART_Receive(ctx->hw.uart, buf, expected_rx_size, STACK_RECV_TIMEOUT)) != 0){
        return status;
    }
    RxStackFrame rx_frames[N_STACKDEVS];
    FillStackFrames(rx_frames, buf, sizeof(uint8_t), N_STACKDEVS);

    //Parsing through received Rx frames and checking the FAULT_OTP registers of each monitor chip
    for (size_t i = 0; i < N_STACKDEVS; i++){
        if (rx_frames[i].data[0] != 0x00){
            // Returning the device number in the stack that is having the error
            return 1;
        }
    }
    // Returning 0 if no errors found in any monitor chip when loading OTP
    return 0;
}

int UnlockNVM(DbmsCtx* ctx)
{
    int status = 0;
    //step 1: retreive FAULT_OTP data (Stack read command) and check for errors
    if (FaultOtpErrorCheck(ctx) != 0){
        return 1;
    }
    //step 2: sending write commands to OTP_PROG_UNLOCK** registers with specific data

    // OTP_PROG_UNLOCK1A..D combined into one write command frame
    uint8_t* buffer1 = {0xB3, 0x03, 0x00, 0x02, 0xB7, 0x78, 0xBC, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, buffer1, sizeof(buffer1))) != 0){
        return status;
    }

    //OTP_PROG_UNLOCK2A..D combined into one write command frame
    uint8_t* buffer2 = {0xB3, 0x03, 0x52, 0x7E, 0x12, 0x08, 0x6F, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, buffer2, sizeof(buffer2))) != 0){
        return status;
    }
    
    //Confirm unlock stack read command
    uint8_t* confirm_unlock = {0xA0, 0x05, 0x19, 0x00, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, confirm_unlock, sizeof(confirm_unlock))) != 0){
        return status;
    }
    // Receiving OTP_PROG_STAT data from monitor chips
    size_t expected_rx_size = RX_FRAME_SIZE(sizeof(uint8_t)) * N_STACKDEVS;
    uint8_t* rx_buffer[expected_rx_size];
    RxStackFrame rx_frames[N_STACKDEVS];

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer, expected_rx_size, STACK_RECV_TIMEOUT)) != 0){
        return status;
    }
    FillStackFrames(rx_frames, rx_buffer, sizeof(uint8_t), N_STACKDEVS);

    for (size_t i = 0; i < N_STACKDEVS; i++){
        //UNLOCK bit in the OTP_PROG_STAT[7] register is 1 = successful unlock
        if (rx_frames[i].data != 0x80){
            return 1;
        }
    }

    return 0;
}

int OTPProgrammingErrorCheck(DbmsCtx* ctx){
    int status = 0;

    // step 1: read registers OTP_PROG_STAT, OTP_CUST1_STAT and OTP_CUST2_STAT
    // send read command frame
    uint8_t* buffer = {0xA0, 0x05, 0x19, 0x02, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, buffer, sizeof(buffer))) != 0){
        return status;
    }

    // receive response frames
    size_t data_size = sizeof(uint8_t) * 3;
    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_STACKDEVS;
    uint8_t* rx_buffer[expected_rx_size];

    RxStackFrame rx_frames[N_STACKDEVS];
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer, expected_rx_size, STACK_RECV_TIMEOUT)) != 0){
        return status;
    }
    FillStackFrames(rx_frames, rx_buffer, data_size, N_STACKDEVS);

    // Check corresponding error register bits
    for (size_t i = 0; i < N_STACKDEVS; i++){
        if (rx_frames[i].data[0] != 0x01){
            return 1;
        }
        if (rx_frames[1].data[1] != 0x0F){
            return 1;
        }
        if (rx_frames[i].data[2] != 0x00){
            return 1; // Although we are not loading page 2 at all, doesnt hurt to check
        }
    }
    
    return status;

}

int TurnMonitorChipLedOn(DbmsCtx* ctx)
{   
    // Can skip a couple steps when doing this over and over again, such as:
    // Doesn't need to check specific page faults (page 1 or 2) every time if theres no programming in page
    int status = 0;
    // step 1: unlock NVM memory programming
    // if ((status = UnlockNVM(ctx)) != 0){
    //     return status;
    // }
    
    //step 2: select OTP page to program
    uint8_t* buffer = {0xB0, 0x03, 0x0B, 0x01, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, buffer, sizeof(buffer))) != 0){
        return status;
    }

    //step 3: program GPIO_CONF4[5:3] = 100 for high and 101 for low
    buffer[1] = 0x00;
    buffer[2] = 0x11;
    buffer[3] = 0x20;
    if ((status = SendStackFrameSetCrc(ctx, buffer, sizeof(buffer))) != 0){
        return status;
    }

    DelayUs(ctx, 100);

    //step 4: Confirm successful OTP page programming with no errors
    if ((status = OTPProgrammingErrorCheck(ctx)) != 0){
        return status;
    }

    //step 5: digital reset and reload registers with updated OTP values
    buffer[1] = 0x03;
    buffer[2] = 0x09;
    buffer[3] = 0x02;
    if ((status = SendStackFrameSetCrc(ctx, buffer, sizeof(buffer))) != 0){
        return status;
    }
}
