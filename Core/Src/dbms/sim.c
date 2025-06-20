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
//            Simulation Networking                      
//---------------------------------------------------

int NetConnect(NetCtx* ctx, const char* ip, uint16_t port) {
    ctx->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sockfd < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) return -1;

    if (connect(ctx->sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
    return 0;
}

int NetSend(NetCtx* ctx, const uint8_t* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(ctx->sockfd, data + sent, size - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

size_t NetRecv(NetCtx* ctx, uint8_t* data, size_t max_size, int timeout_ms) {
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(ctx->sockfd, &fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(ctx->sockfd + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0) return 0;

    return recv(ctx->sockfd, data, max_size, 0);
}

void NetClose(NetCtx* ctx) {
    close(ctx->sockfd);
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
    if (!NetConnect(&hcan->net, NET_LOCALHOST, NET_CAN_PORT)) 
        return HAL_ERROR;
    return HAL_OK;  
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan)
{
    return 0;       // explicit do nothing
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[], uint32_t *pTxMailbox)
                                       
{
    // printf("CAN | %03x | %02x | %02x | %02x | %02x | %02x | %02x | %02x | %02x\n",
        // pHeader->StdId, aData[0], aData[1], aData[2], aData[3], aData[4], aData[5], aData[6], aData[7]);

    uint8_t framebuf[12] = {0};
    memcpy(framebuf, &pHeader->StdId, 4);
    memcpy(framebuf + 4, aData, 8);

    if (!NetSend(&hcan->net, framebuf, 12))
        return HAL_ERROR;
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

void __SimCleanup(CAN_HandleTypeDef* hcan)
{
    NetClose(&hcan->net);
}

#endif 