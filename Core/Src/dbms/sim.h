//
//  Copyright (C) Texas A&M University
//
//  Simulated Hardware Abstraction Layer
//
#ifndef _SIM_H_
#define _SIM_H_

#ifdef SIM

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// DO NOT INCLUDE CONTEXT OR COMMON HERE

//---------------------------------------------------
//            Simulation Networking
//---------------------------------------------------

#define NET_LOCALHOST   "127.0.0.1"
#define NET_UART_PORT   1476
#define NET_CAN_PORT    1477

typedef struct {
    int sockfd;
} NetCtx;

int NetConnect(NetCtx *ctx, const char *ip, uint16_t port);

int NetSend(NetCtx *ctx, const uint8_t *data, size_t size);

size_t NetRecv(NetCtx *ctx, uint8_t *data, size_t max_size, int timeout_ms);

void NetClose(NetCtx *ctx);

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
    NetCtx net;
} UART_HandleTypeDef;

typedef struct {
    NetCtx net;
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

void __SimCleanup(CAN_HandleTypeDef *hcan);

#endif
#endif