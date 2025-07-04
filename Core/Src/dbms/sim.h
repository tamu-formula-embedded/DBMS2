//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _SIM_H_
#define _SIM_H_

#ifdef SIM

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// DO NOT INCLUDE CONTEXT OR COMMON HERE


//---------------------------------------------------
//            Simulation & IPC 
//---------------------------------------------------

typedef struct {
    int ipc_fd_can;
    int ipc_fd_uart;
    bool can_started;
} __SimCtx;

void __SimEnter(char* ipc_path_can, char* ipc_path_uart);
void __SimExit();

int __SimIpcCreate(const char* port_path, int baud_rate);

int __SimIpcSend(int fd, const unsigned char* data, int size);

//---------------------------------------------------
//            HAL Spoof
//---------------------------------------------------

#define CAN_FILTERMODE_IDMASK 0
#define CAN_FILTERSCALE_32BIT 0
#define CAN_RX_FIFO0 0
#define ENABLE 0
#define CAN_ID_STD 0
#define CAN_RTR_DATA 0
#define DISABLE 0
#define USART_CR1_UE 0


#undef CR1
struct {
    int CR1;
    int BRR;
}* UART4;

typedef enum {
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    int _;
} ADC_HandleTypeDef;
typedef struct {
    int _;
} TIM_HandleTypeDef;
typedef struct {
    int _;
} UART_HandleTypeDef;
typedef struct {
    int _;
} CAN_HandleTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t FilterIdHigh;
    uint32_t FilterIdLow;
    uint32_t FilterMaskIdHigh;
    uint32_t FilterMaskIdLow;
    uint32_t FilterFIFOAssignment;
    uint32_t FilterBank;
    uint32_t FilterMode;
    uint32_t FilterScale;
    uint32_t FilterActivation;
    uint32_t SlaveStartFilterBank;
} CAN_FilterTypeDef;

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan,
                                       const CAN_FilterTypeDef *sFilterConfig);

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan);

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan,
                                       const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[],
                                       uint32_t *pTxMailbox);

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc,
                                            uint32_t Timeout);

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

void HAL_Delay(uint32_t Delay);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout);

void HAL_TIM_Base_Start(TIM_HandleTypeDef *huart);

#endif
#endif