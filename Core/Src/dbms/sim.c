//  
//  Copyright (c) Texas A&M University.
//  
#ifdef SIM

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __clang__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif

#include "sim.h"

//---------------------------------------------------
//            Simulation & IPC                      
//---------------------------------------------------

static __SimCtx __sim_ctx;

void __SimEnter(char* ipc_path_can, char* ipc_path_uart)
{
    __sim_ctx.ipc_fd_can = __SimIpcCreate(ipc_path_can, 1000000);
    __sim_ctx.ipc_fd_uart = __SimIpcCreate(ipc_path_uart, 1000000);
    __sim_ctx.can_started = false;
}
void __SimExit()
{
    close(__sim_ctx.ipc_fd_can);
    close(__sim_ctx.ipc_fd_uart);
}

int __SimIpcCreate(const char* port_path, int baud_rate) {
    int fd = open(port_path, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        fprintf(stderr, "IPC: error opening serial port %s.\n", port_path);
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, baud_rate);
    cfsetospeed(&options, baud_rate);

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_cflag &= ~CRTSCTS;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

int __SimIpcSend(int fd, const unsigned char* data, int size) {
    int total_written = 0;
    while (total_written < size) {
        int written = write(fd, data + total_written, size - total_written);
        if (written < 0) {
            perror("IPC: error writing to serial port");
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

//---------------------------------------------------
//            HAL Spoof                      
//---------------------------------------------------

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *sFilterConfig)
{
    return HAL_OK;  // explicit do nothing
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
    __sim_ctx.can_started = true;       // this is evil!
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan)
{
    return 0;       // explicit do nothing
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[], uint32_t *pTxMailbox)
                                       
{
    if (!__sim_ctx.can_started) return HAL_BUSY;
    // printf("CAN | %03x | %02x | %02x | %02x | %02x | %02x | %02x | %02x | %02x\n",
        // pHeader->StdId, aData[0], aData[1], aData[2], aData[3], aData[4], aData[5], aData[6], aData[7]);

    uint8_t framebuf[12] = {0};
    uint32_t id = pHeader->StdId;
    framebuf[0] = (id >> 24) & 0xFF;
    framebuf[1] = (id >> 16) & 0xFF;
    framebuf[2] = (id >> 8) & 0xFF;
    framebuf[3] = id & 0xFF;
    memcpy(framebuf + 4, aData, 8);

    __SimIpcSend(__sim_ctx.ipc_fd_can, framebuf, 12);

    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    return HAL_OK;  // explicit do nothing
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout)
{
    return HAL_OK;  // explicit do nothing
}

static int i = 0;

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    i++;
    i %= 1000;
    return i;
}

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc)
{
    return HAL_OK;  // explicit do nothing
}

void HAL_Delay(uint32_t Delay)
{
    usleep(Delay * 1000);
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    // TODO: Wake logic if BRR is set
    __SimIpcSend(__sim_ctx.ipc_fd_uart, pData, Size);
    usleep(100);
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)read(__sim_ctx.ipc_fd_uart, pData, Size);
}

void HAL_TIM_Base_Start(TIM_HandleTypeDef *huart)
{

}

#endif 