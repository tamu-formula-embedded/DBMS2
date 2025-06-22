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