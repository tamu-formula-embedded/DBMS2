//  
//  Copyright (c) Texas A&M University.
//  
#include "vehicle_interface.h"

int ConfigCan(HwCtx* hw_ctx)
{
    int status = HAL_OK;

    // Create a filter
    CAN_FilterTypeDef s_filter_cfg = {0};
//    s_filter_cfg.FilterBank = 0;
//    s_filter_cfg.FilterMode = CAN_FILTERMODE_IDMASK;
//    s_filter_cfg.FilterScale = CAN_FILTERSCALE_32BIT;
//    s_filter_cfg.FilterIdHigh = 0x0000;
//    s_filter_cfg.FilterIdLow = 0x0000;
//    s_filter_cfg.FilterMaskIdHigh = 0x0000;
//    s_filter_cfg.FilterMaskIdLow = 0x0000;
//    s_filter_cfg.FilterFIFOAssignment = CAN_RX_FIFO0;
//    s_filter_cfg.FilterActivation = ENABLE;
//    s_filter_cfg.SlaveStartFilterBank = 14;

    s_filter_cfg.FilterBank = 14;  // For CAN2, banks 14–27 are available
    s_filter_cfg.FilterMode = CAN_FILTERMODE_IDMASK;
    s_filter_cfg.FilterScale = CAN_FILTERSCALE_32BIT;
    s_filter_cfg.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    s_filter_cfg.FilterActivation = ENABLE;
    s_filter_cfg.SlaveStartFilterBank = 14;  // Required when using CAN2

    // Accept all 11-bit standard IDs (0x000–0x7FF), reject extended frames
    s_filter_cfg.FilterIdHigh = 0x0000;              // ID[28:13]
    s_filter_cfg.FilterIdLow  = 0x0000;              // ID[12:0], IDE=0, RTR=0
    s_filter_cfg.FilterMaskIdHigh = 0x0000;          // Mask to allow all standard IDs
    s_filter_cfg.FilterMaskIdLow  = 0x0004;          // Mask IDE bit to reject extended frames

    // Add the filter to CAN peripheral
    if ((status = HAL_CAN_ConfigFilter(hw_ctx->can, &s_filter_cfg)) != HAL_OK) return status;

    if ((status = HAL_CAN_Start(hw_ctx->can)) != HAL_OK) return status;
    if ((status = HAL_CAN_ActivateNotification(hw_ctx->can, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING)) != HAL_OK) 
        return status;

    // Config TX header
    hw_ctx->can_tx_header.StdId = 0xdead;
    hw_ctx->can_tx_header.IDE = CAN_ID_STD;
    hw_ctx->can_tx_header.RTR = CAN_RTR_DATA;
    hw_ctx->can_tx_header.DLC = 8;
    hw_ctx->can_tx_header.TransmitGlobalTime = DISABLE;

    return status;
}

int CanTransmit(HwCtx* hw_ctx, uint32_t id, uint8_t data[8])
{
    hw_ctx->can_tx_header.StdId = id;
    int32_t result = HAL_CAN_AddTxMessage(hw_ctx->can, &hw_ctx->can_tx_header, data, &hw_ctx->can_tx_mailbox);
    return result;
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

#define CELL_STATUS_VOLTAGE     0x5F4
#define CELL_STATUS_TEMP        0x5F5

void DumpCellState(DbmsCtx* ctx, HwCtx* hw)
{
    uint8_t frame[8] = {0};

    for (size_t i = 0; i < N_SEGMENTS; i++)
    {
        frame[0] = i;
        for (size_t j = 0; j < N_MONITORS_PER_SEG; j++)
        {
            frame[1] = j;
            for (size_t k = 0; k < N_GROUPS; k++)
            {
                frame[2] = k;
                uint16_t v = ctx->cell_states[i][j].voltages[k];
                frame[6] = (v & 0xff00) >> 8;
                frame[7] = (v & 0x00ff);
                CanTransmit(hw, CELL_STATUS_VOLTAGE, frame);
            }
            for (size_t k = 0; k < N_TEMPS; k++)
            {
                frame[2] = k;
                uint16_t t = ctx->cell_states[i][j].temps[k];
                frame[6] = (t & 0xff00) >> 8;
                frame[7] = (t & 0x00ff);
                CanTransmit(hw, CELL_STATUS_TEMP, frame);
            }
        }
    }
}
