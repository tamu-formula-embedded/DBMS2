//  
//  Copyright (c) Texas A&M University.
//  
#include "stack_controller.h"

const static uint16_t crc16_table[256] = {0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301,
                                          0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1,
                                          0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81,
                                          0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
                                          0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00,
                                          0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1,
                                          0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380,
                                          0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141,
                                          0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501,
                                          0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0,
                                          0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881,
                                          0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
                                          0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401,
                                          0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1,
                                          0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180,
                                          0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
                                          0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01,
                                          0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1,
                                          0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80,
                                          0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
                                          0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200,
                                          0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1,
                                          0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780,
                                          0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41,
                                          0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
                                          0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1,
                                          0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80,
                                          0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
                                          0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};


static uint8_t FRAME_WAKE_STACK[] = { 0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95 };
static uint8_t FRAME_SHUTDOWN_STACK[] = { 0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00 };

void DelayUs(HwCtx* hw, uint16_t us)
{
#ifdef SIM
    usleep(us);
#else
    __HAL_TIM_SET_COUNTER(hw->timer, 0);
    uint64_t big_ctr;
    while ((big_ctr = __HAL_TIM_GET_COUNTER(hw->timer)) < us);
#endif
}

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

uint16_t CalcCrc16(uint8_t *buf, size_t len)
{
    int w_crc = 0xFFFF, i;
    for (i = 0; i < len; i++)
    {
        w_crc ^= (uint16_t)(*buf++) & 0x00FF;
        w_crc = crc16_table[w_crc & 0x00FF] ^ (w_crc >> 8);
    }
    return w_crc;
}

int SendStackFrame(HwCtx* hw, uint8_t* buf, size_t len)
{
    return HAL_UART_Transmit(hw->uart, buf, len, STACK_SEND_TIMEOUT);
}

int SendStackFrameSetCrc(HwCtx* hw, uint8_t* buf, size_t len)
{
    int status = 0;
	uint16_t crc = CalcCrc16(buf, len - 2);
    buf[len - 2] = crc & 0xFF;
    buf[len - 1] = (crc >> 8) & 0xFF;
    status = HAL_UART_Transmit(hw->uart, buf, len, STACK_SEND_TIMEOUT);
    buf[len - 2] = 0;
    buf[len - 1] = 0;
    return status;
}

void RecvStackFrame(HwCtx* hw, RxStackFrame* rx_frame)
{
    HAL_UART_Receive(hw->uart, rx_frame->buffer, rx_frame->size + 6, STACK_RECV_TIMEOUT);
    rx_frame->init_field = rx_frame->buffer[0];
    rx_frame->dev_addr = rx_frame->buffer[1];
    rx_frame->reg_addr = (rx_frame->buffer[2] << 8) + rx_frame->buffer[3];
}

void SetBrr(uint64_t brr)
{
#ifndef SIM
    UART4->CR1 &= ~(USART_CR1_UE);
    UART4->BRR = brr;
    UART4->CR1 |= USART_CR1_UE;
#endif
}

int SendStackWakeBlip(HwCtx* hw)
{
    int status = 0;
    // uart_set_brr(APBxCLK / (1 / (2.75 / (1 * 1000))));
    SetBrr(25700/2); // ?
    uint8_t wake_frame[] = {0x00};
    if ((status = SendStackFrame(hw, wake_frame, sizeof(wake_frame))) != 0) return status;
    SetBrr(APBxCLK / 1000000); // 84
    DelayUs(hw, 3500);
    return status;
}

int SendStackShutdownBlip(HwCtx* hw)
{
    int status = 0;
    // uart_set_brr(APBxCLK / (1 / (2.75 / (1 * 1000))));
    SetBrr(128000);
    uint8_t wake_frame[] = {0x00};
    if ((status = SendStackFrame(hw, wake_frame, sizeof(wake_frame))) != 0) return status;
    SetBrr(APBxCLK / 1000000); // 84
    DelayUs(hw, 3500);
    return status;
}


int StackWake(HwCtx* hw)
{
    int status = HAL_OK;
    for (int i = 0; i < 2; i++) 
    {
        if ((status = SendStackWakeBlip(hw)) != HAL_OK)
            return status;
        if ((status = SendStackFrame(hw, FRAME_WAKE_STACK, sizeof(FRAME_WAKE_STACK))) != HAL_OK)
            return status;
        HAL_Delay(15 + 12 * N_STACKDEVS);   // wtf
    }
    return status;
}

int StackShutdown(HwCtx* hw)
{
    int status = HAL_OK;
    for (int i = 0; i < 2; i++) 
    {
    	if ((status = SendStackFrameSetCrc(hw, FRAME_SHUTDOWN_STACK, sizeof(FRAME_SHUTDOWN_STACK))) != HAL_OK)
    	    return status;
    	if ((status = SendStackShutdownBlip(hw)) != HAL_OK)
            return status;
        HAL_Delay(100);
    }
    return status;
}

void SendOtpEccDatain(HwCtx* hw)  
{
    // uint8_t frame_otp_ecc_datain[] = { 0xB0, 0x03, 0x43, 0x00, 0x00, 0x00 };
    uint8_t frame_otp_ecc_datain[] = { 0xD0, 0x03, 0x4C, 0x00, 0x00, 0x00 };
    for (int i = 0; i < 8; i++) 
    {
        SendStackFrameSetCrc(hw, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;   
    }
}

void SendAutoAddr(HwCtx* hw) 
{
    // 0xB0 at first
    uint8_t frame_addr_dev[] = { 0xD0, 0x03, 0x06, 0x00, 0x00, 0x00 };
    for (int i = 0; i <= N_STACKDEVS; i++) {
        SendStackFrameSetCrc(hw, frame_addr_dev, sizeof(frame_addr_dev));
        frame_addr_dev[3]++;   
    }
}

void SendSetStackTop(HwCtx* hw) 
{
    // 2nd byte replaced with id (num of stack devs), last 2 bytes for crc
    uint8_t frame_set_stack_base[] = { 0x90, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00 }; 
    uint8_t frame_set_stack_top[] = { 0x90, N_STACKDEVS-1, 0x03, 0x08, 0x03, 0x00, 0x00 }; 
    SendStackFrameSetCrc(hw, frame_set_stack_base, sizeof(frame_set_stack_base));
    SendStackFrameSetCrc(hw, frame_set_stack_top, sizeof(frame_set_stack_top));
}

void ReadOtpEccDatain(HwCtx* hw)  
{
    // uint8_t frame_otp_ecc_datain[] = { 0xA0, 0x03, 0x43, 0x00, 0x00, 0x00};
    uint8_t frame_otp_ecc_datain[] = { 0xC0, 0x03, 0x4C, 0x00, 0x00, 0x00};
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(hw, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}

void StackAutoAddr(HwCtx* hw)
{
    SendOtpEccDatain(hw); // step 1

    static uint8_t FRAME_ENABLE_AUTO_ADDR[] = { 0xD0, 0x03, 0x09, 0x01, 0x0F, 0x74 };
    SendStackFrameSetCrc(hw, FRAME_ENABLE_AUTO_ADDR, sizeof(FRAME_ENABLE_AUTO_ADDR)); // step 2

    SendAutoAddr(hw); // step 3

    static uint8_t FRAME_SET_ALL_STACK[] = { 0xD0, 0x03, 0x08, 0x02, 0x0, 0x0 };
    SendStackFrameSetCrc(hw, FRAME_SET_ALL_STACK, sizeof(FRAME_SET_ALL_STACK)); // step 4

    SendSetStackTop(hw); // step 5

    ReadOtpEccDatain(hw); // step 6

    static uint8_t FRAME_READ_DIR0_ADDR[] = { 0xA0, 0x00, 0x03, 0x06, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(hw, FRAME_READ_DIR0_ADDR, sizeof(FRAME_READ_DIR0_ADDR));

    static uint8_t FRAME_CONF1_READ[] = { 0x80, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(hw, FRAME_CONF1_READ, sizeof(FRAME_CONF1_READ));
}

void StackSetNumActiveCells(HwCtx* hw, uint8_t n_active_cells)
{
    // possible sw guide error 0xB0 (stack write) --> 0xD0 (broadcast write)
    uint8_t frame_set_active_cell[] =  { 0xD0, 0x00, N_STACKDEVS, n_active_cells, 0x00, 0x00 };

    SendStackFrameSetCrc(hw, frame_set_active_cell, sizeof(frame_set_active_cell));
}

void StackSetupGpio(HwCtx* hw)
{
    // TODO: implement
}

void StackSetupTempReadings(HwCtx* hw)
{

}

void StackSetupVoltReadings(HwCtx* hw)
{
    uint8_t FRAME_SET_ADC_CONT_RUN[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00 };
    SendStackFrameSetCrc(hw, FRAME_SET_ADC_CONT_RUN, sizeof(FRAME_SET_ADC_CONT_RUN));
}

void StackUpdateVoltReadings(HwCtx* hw, DbmsCtx* ctx)
{
    uint8_t FRAME_ADC_MEASUREMENTS[] = { 0xC0, 0x05, 0x68 + 2*(16-N_GROUPS), N_GROUPS*2-1, 0x00, 0x00 };

    SendStackFrameSetCrc(hw, FRAME_ADC_MEASUREMENTS, sizeof(FRAME_ADC_MEASUREMENTS));

    uint8_t addr;
    STACK_DEFINE_RX_FRAME(rx_frame, N_GROUPS * sizeof(int16_t))
    for (size_t i = 0; i < N_STACKDEVS; i++)
    {
        RecvStackFrame(hw, &rx_frame);                      // recv data into the frame
        if ((addr = rx_frame.dev_addr - 1) < 0) continue;  // skip myself
        for (size_t j = 0; j < N_GROUPS; j++)
        {
            ctx->cell_states[addr / N_MONITORS_PER_SEG][addr % N_MONITORS_PER_SEG].voltages[j]
                = (rx_frame.data[j * sizeof(int16_t)] << 8) 
                + (rx_frame.data[j * sizeof(int16_t) + 1]);     // watch out! plus 1 inside
        }
    }
}
