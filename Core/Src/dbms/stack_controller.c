//  
//  Copyright (c) Texas A&M University.
//  
#include "stack_controller.h"


static uint8_t FRAME_WAKE_STACK[] = { 0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95 };
static uint8_t FRAME_SHUTDOWN_STACK[] = { 0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00 };

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
    uint8_t frame_otp_ecc_datain[] = { 0xD0, 0x03, 0x43, 0x00, 0x00, 0x00 };
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

    // Sets all devices as stack devices
    uint8_t frame_set_stack_devices[] = {0xD0, 0x03, 0x08 , 0x02, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_devices, sizeof(frame_set_stack_devices));

    // Sets bridge device as non-stack device and bottom of stack
    uint8_t frame_set_stack_base[] = { 0x90, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00 }; 
    SendStackFrameSetCrc(ctx, frame_set_stack_base, sizeof(frame_set_stack_base));

    // Sets top of stack
    uint8_t frame_set_stack_top[] = { 0x90, N_STACKDEVS-1, 0x03, 0x08, 0x03, 0x00, 0x00 }; 
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

    // static uint8_t FRAME_READ_DIR0_ADDR[] = { 0xA0, 0x00, 0x03, 0x06, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_READ_DIR0_ADDR, sizeof(FRAME_READ_DIR0_ADDR));

    // static uint8_t FRAME_CONF1_READ[] = { 0x80, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_CONF1_READ, sizeof(FRAME_CONF1_READ));
}

/**
 * Sets the number of active cells in the stack
 * Used to initalize the stack
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells)
{
    // possible sw guide error 0xB0 (stack write) --> 0xD0 (broadcast write)
    // uint8_t frame_set_active_cell[] =  { 0xD0, 0x00, N_STACKDEVS, n_active_cells, 0x00, 0x00 };
    uint8_t frame_set_active_cell[] =  { 0xD0, 0x00, 0x03, n_active_cells, 0x00, 0x00 };

    SendStackFrameSetCrc(ctx, frame_set_active_cell, sizeof(frame_set_active_cell));
}

void StackSetupGpio(DbmsCtx* ctx)
{
    // TODO: implement
    // Configures all GPIOs to be ADC and OTUT input
    // Note: Can be made more effecient by using NVM
    // Note 2: Also configures GPIO8 even though we dont need to, hence just setting GPIO8 to output low
    uint8_t frame_gpio_configs[] = {0xB3, 0x00, 0x0E, 0x09, 0x09, 0x09, 0x21, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_gpio_configs, sizeof(frame_gpio_configs));

    // Setting up TSREF to active
//    uint8_t frame_tsref_setup[] = {0xB0, 0x03, 0x0A, 0x01, 0x00, 0x00};
//    SendStackFrameSetCrc(ctx, frame_tsref_setup, sizeof(frame_tsref_setup));
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
    static uint8_t rx_buffer_volt_readings[1024*4];

    int status = 0;
    uint8_t FRAME_ADC_MEASUREMENTS[] = { 0xA0, 0x05, 0x68 + 2*(16-N_GROUPS_PER_SIDE), N_GROUPS_PER_SIDE*2-1, 0x00, 0x00 };

    if ((status = SendStackFrameSetCrc(ctx, FRAME_ADC_MEASUREMENTS, sizeof(FRAME_ADC_MEASUREMENTS))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }

    size_t data_size = N_GROUPS_PER_SIDE * sizeof(int16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_STACKDEVS; // <- num frames
    //                 header+crc ^         ^ readings * 2 bytes each    

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_volt_readings, expected_rx_size, STACK_RECV_TIMEOUT)) != 0)
    {
        // CAN_REPORT_FAULT(ctx, status);
        // ^ throwing all the time in simulator
    }

    RxStackFrame rx_frames[N_STACKDEVS];
    FillStackFrames(rx_frames, rx_buffer_volt_readings, data_size, N_STACKDEVS);

    uint8_t addr;
    for (size_t i = 0; i < N_STACKDEVS; i++)
    {
        if (rx_frames[i].dev_addr == 0) continue;   // this is myself
        addr = rx_frames[i].dev_addr - 1;           // ignore the controller from a broadcast   
        if (addr >= N_MONITORS) continue;            // throw some error here
        if (addr % 2 == 1) continue;                // skip the odds
        addr /= 2;                                  // addr now in side space
        if (addr >= N_SIDES) continue;              // throw some error here
        
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            uint16_t raw = (rx_frames[i].data[j * sizeof(int16_t)] << 8) 
                         + (rx_frames[i].data[j * sizeof(int16_t) + 1]);     
            
            ctx->cell_states[addr].voltages[j] = (raw * STACK_V_UV_PER_BIT) / 1000.0;    // floating mV
        } 
    }
}

void StackUpdateTempReadings(DbmsCtx* ctx)
{
    // Retreives GPIO1 through GPIO7 values and stores them into the cell_states->temps array
    int status = 0;
    // Send read command to read GPIO1..7
    uint8_t frame_read_gpio[] = {0xA0, 0x05, 0x8E, 0x0D, 0x00, 0x00}; // Reading 14 bytes
    SendStackFrameSetCrc(ctx, frame_read_gpio, sizeof(frame_read_gpio));

    // Receive response frame
    size_t data_size = N_TEMPS_PER_SIDE * sizeof(uint16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_MONITORS;
    static uint8_t rx_frame[1024*4];

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_frame, expected_rx_size, STACK_RECV_TIMEOUT)) != 0){
        //Error
    }
    RxStackFrame rx_frames[N_MONITORS];
    FillStackFrames(rx_frames, rx_frame, data_size, N_MONITORS);
    int c = 0;
    // Store data into cell_states->temps
    for (int i = 0; i < N_MONITORS; i++){
//        memcpy_eswap2(ctx->cell_states[i].temps, rx_frames[i].data, data_size);
//        uint8_t addr = rx_frames[i].dev_addr - 1;
//        CanLog(ctx, "a %d\n", addr);
        c++;

        for (size_t j = 0; j < N_TEMPS_PER_SIDE; j++)
        {

            uint16_t raw = (rx_frames[i].data[j * sizeof(int16_t)] << 8) 
                         + (rx_frames[i].data[j * sizeof(int16_t) + 1]);    
            
            ctx->cell_states[i].temps[j] = (float)raw; // todo: conversion
        } 
    }
//    CanLog(ctx, "c %d", c);
}

int ToggleMonitorChipLed(DbmsCtx* ctx, bool on, uint8_t dev_number)
{   
    // Turns on or off LED connected to GPIO8 on the specified monitor chip
    int status = 0;
    uint8_t on_off_value = 0x28; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x20;
    uint8_t led_change_write_com[] = {0x90, dev_number, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send single device write command frame
    if ((status = SendStackFrameSetCrc(ctx, led_change_write_com, sizeof(led_change_write_com))) != 0){
        return status;
    }
    return status;
}

int ToggleAllMonitorChipLeds(DbmsCtx* ctx, bool on){
    // Turns on or off LED connected to GPIO8 on all monitor chips
    // Note for future: 
    int status = 0;
    uint8_t on_off_value = 0x28; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x20;
    uint8_t leds_change_write_com[] = {0xB0, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send stack device write command frame
    if ((status = SendStackFrameSetCrc(ctx, leds_change_write_com, sizeof(leds_change_write_com))) != 0){
        return status;
    }
    return status;
}
// votlages ->  if (addr % 2 == 0)
//                  monitor id = addr / 2
//                  write(0...N_GROUPS_PER_SIDE)

// temps ->     monitor id = addr / 2 
//              if (addr % 2 == 0)
//                  write(0...N_TEMPS_PER_SIDE)
//              else
//                  write(N_TEMPS_PER_SIDE+1...2*N_TEMPS_PER_SIDE)


void MonitorLedBlink(DbmsCtx* ctx){

    uint64_t curr_ts = GetUs(ctx) - ctx->M_LED_BLINK_TS;
//    CanLog(ctx, "%d \n", curr_ts);

    if (ctx->M_LED_ON){
        if (curr_ts > 100000){
            ToggleAllMonitorChipLeds(ctx, false);
            ctx->M_LED_ON = false;
            ctx->M_LED_BLINK_TS = GetUs(ctx);
        }
    }
    else{
        if (curr_ts > 1000000){
            ToggleAllMonitorChipLeds(ctx, true);
            ctx->M_LED_ON = true;
            ctx->M_LED_BLINK_TS = GetUs(ctx);
        }
    }

}
