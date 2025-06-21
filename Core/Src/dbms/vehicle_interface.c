//  
//  Copyright (c) Texas A&M University.
//  
#include "vehicle_interface.h"

bool ConfigCan(HwCtx* hw_ctx)
{
    // Create a filter
    CAN_FilterTypeDef s_filter_cfg = {0};
    s_filter_cfg.FilterBank = 0;
    s_filter_cfg.FilterMode = CAN_FILTERMODE_IDMASK;
    s_filter_cfg.FilterScale = CAN_FILTERSCALE_32BIT;
    s_filter_cfg.FilterIdHigh = 0x0000;
    s_filter_cfg.FilterIdLow = 0x0000;
    s_filter_cfg.FilterMaskIdHigh = 0x0000;
    s_filter_cfg.FilterMaskIdLow = 0x0000;
    s_filter_cfg.FilterFIFOAssignment = CAN_RX_FIFO0;
    s_filter_cfg.FilterActivation = ENABLE;
    s_filter_cfg.SlaveStartFilterBank = 14;

    // Add the filter to CAN peripheral
    if (HAL_CAN_ConfigFilter(hw_ctx->can, &s_filter_cfg) != HAL_OK) return true;

    if (HAL_CAN_Start(hw_ctx->can) != HAL_OK) return true;

    // Config TX header
    hw_ctx->can_tx_header.StdId = 0xdead;
    hw_ctx->can_tx_header.IDE = CAN_ID_STD;
    hw_ctx->can_tx_header.RTR = CAN_RTR_DATA;
    hw_ctx->can_tx_header.DLC = 8;
    hw_ctx->can_tx_header.TransmitGlobalTime = DISABLE;

    return false;
}

bool CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8])
{
    hw_ctx->can_tx_header.StdId = id;
    int32_t result = HAL_CAN_AddTxMessage(hw_ctx->can, &hw_ctx->can_tx_header, data, &hw_ctx->can_tx_mailbox);
    if (result != HAL_OK) return true;
    return false;
}

void CanLog(HwCtx* hw_ctx, const char* fmt, ...) 
{
    char buffer[CAN_LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) return;

    for (int i = 0; i < len; i += 8) 
    {
        uint8_t chunk[8] = {0}; // zero out to pad shorter chunks
        int chunk_size = (len - i >= 8) ? 8 : (len - i);
        memcpy(chunk, &buffer[i], chunk_size);
        CanTransmit(hw_ctx, CAN_LOG_ID, chunk);
    }
}