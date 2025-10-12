//
//  Copyright (c) Texas A&M University.
//
#ifndef _SIM_H_
#define _SIM_H_

#ifdef SIM

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

// DO NOT INCLUDE CONTEXT OR COMMON HERE

//---------------------------------------------------
//            Simulation & IPC
//---------------------------------------------------

typedef struct
{
    int ipc_fd_can;
    int ipc_fd_uart;
    bool can_started;
} __SimCtx;

void __SimEnter(char* ipc_path_can, char* ipc_path_uart);
void __SimExit();

int __SimIpcSend(int fd, const unsigned char* data, int size);

// // Poll the CAN socket and enqueue any complete frames found (non-blocking)
// void __SimCanPoll(void);

// // Pop one frame from the internal queue.
// // Returns 1 if a frame was returned in *out, 0 if queue empty.
// int __SimCanPop(SimCanFrame* out);

//---------------------------------------------------
//            HAL Spoof
//---------------------------------------------------

#define CAN_FILTERMODE_IDMASK 1
#define CAN_FILTERMODE_IDLIST 0
#define CAN_FILTERSCALE_32BIT 1
#define CAN_FILTERSCALE_16BIT 0
#define CAN_RX_FIFO0 0
#define ENABLE 1
#define CAN_ID_STD 0
#define CAN_RTR_DATA 0
#define DISABLE 0
#define USART_CR1_UE 0

#define CAN_IT_RX_FIFO0_MSG_PENDING 0
#define CAN_IT_RX_FIFO1_MSG_PENDING 0
#define CAN_FILTER_FIFO0 0

#undef CR1
struct
{
    int CR1;
    int BRR;
}* UART4;

typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct
{
    int _;
} ADC_HandleTypeDef;
typedef struct
{
    int _;
} TIM_HandleTypeDef;
typedef struct
{
    int _;
} UART_HandleTypeDef;
typedef struct
{
    int _;
} CAN_HandleTypeDef;
typedef struct
{
    int _;
} GPIO_TypeDef;
typedef struct
{
    int _;
} I2C_HandleTypeDef;

#define GPIOC NULL
#define GPIOA NULL
#define GPIO_PIN_6 6
#define GPIO_PIN_7 7
#define GPIO_PIN_8 8
#define GPIO_PIN_9 9

HAL_StatusTypeDef HAL_GPIO_WritePin(GPIO_TypeDef* x, int p, int a);

typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct
{
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

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef* hcan, const CAN_FilterTypeDef* sFilterConfig);

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef* hcan);

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef* hcan);

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef* hcan, const CAN_TxHeaderTypeDef* pHeader,
                                       const uint8_t aData[], uint32_t* pTxMailbox);

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);

void HAL_Delay(uint32_t Delay);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, const uint8_t* pData, uint16_t Size, uint32_t Timeout);

void HAL_TIM_Base_Start(TIM_HandleTypeDef* huart);

typedef struct
{
    uint32_t StdId; /*!< Specifies the standard identifier.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF. */

    uint32_t ExtId; /*!< Specifies the extended identifier.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF. */

    uint32_t IDE; /*!< Specifies the type of identifier for the message that will be transmitted.
                       This parameter can be a value of @ref CAN_identifier_type */

    uint32_t RTR; /*!< Specifies the type of frame for the message that will be transmitted.
                       This parameter can be a value of @ref CAN_remote_transmission_request */

    uint32_t DLC; /*!< Specifies the length of the frame that will be transmitted.
                       This parameter must be a number between Min_Data = 0 and Max_Data = 8. */

    uint32_t Timestamp; /*!< Specifies the timestamp counter value captured on start of frame reception.
                            @note: Time Triggered Communication Mode must be enabled.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 0xFFFF. */

    uint32_t FilterMatchIndex; /*!< Specifies the index of matching acceptance filter element.
                            This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF. */

} CAN_RxHeaderTypeDef;

int HAL_GetTick();

int HAL_I2C_Master_Transmit(I2C_HandleTypeDef* _, int addr, char* buf, int size, int timeout);
int HAL_I2C_Master_Receive(I2C_HandleTypeDef* _, int addr, char* buf, int size, int timeout);

int HAL_CAN_ActivateNotification(CAN_HandleTypeDef* j, int k);

#endif
#endif