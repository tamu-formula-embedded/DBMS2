/** 
 * 
 * Distributed BMS      CAN I/O Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#include "can.h"

typedef struct 
{
    uint32_t id;        // up to 29-bit CAN ID
    uint32_t mask;      // 29-bit or 11-bit mask
    bool extended;      // true = extended (29-bit)
} CanFilterMask;


/**
 * 
 */
int ConfigCanFilters(CAN_HandleTypeDef *hcan, const CanFilterMask *filters, size_t count)
{
    const int base_bank = 14;
    int status = HAL_OK, bank = 0;
    size_t i = 0;

    while (i < count)
    {
        CAN_FilterTypeDef f = {0};
        f.FilterBank = base_bank + bank;
        f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        f.FilterMode = CAN_FILTERMODE_IDMASK;
        f.FilterActivation = ENABLE;
        f.SlaveStartFilterBank = base_bank;

        // Check if this entry (or next) is extended -> use 32-bit mode
        if (filters[i].extended)
        {
            f.FilterScale = CAN_FILTERSCALE_32BIT;
            f.FilterIdHigh     = (filters[i].id   << 3) >> 16;
            f.FilterIdLow      = ((filters[i].id  << 3) & 0xFFFF) | (1 << 2); 
            f.FilterMaskIdHigh = (filters[i].mask << 3) >> 16;
            f.FilterMaskIdLow  = ((filters[i].mask << 3) & 0xFFFF) | (1 << 2);

            status = HAL_CAN_ConfigFilter(hcan, &f);
            if (status != HAL_OK) 
                return status;
            bank++;
            i++;
        }
        else
        {
            f.FilterScale = CAN_FILTERSCALE_16BIT;
            // First standard ID
            f.FilterIdHigh     = filters[i].id   << 5;
            f.FilterMaskIdHigh = filters[i].mask << 5;
            i++;

            // Second standard ID (if available and not extended)
            if (i < count && !filters[i].extended) {
                f.FilterIdLow      = filters[i].id   << 5;
                f.FilterMaskIdLow  = filters[i].mask << 5;
                i++;
            } else {
                f.FilterIdLow = 0;
                f.FilterMaskIdLow = 0;
            }

            status = HAL_CAN_ConfigFilter(hcan, &f);
            if (status != HAL_OK) 
                return status;
            bank++;
        }
    }

    return HAL_OK;
}


int ConfigCan(DbmsCtx* ctx)
{
    int status = 0;
    CanFilterMask masks[] = 
    {
        { 0x0B0, 0x7F0, false },
        { 0x500, 0x700, false },
        { CANID_ELCON_TX, 0, true },
        { CANID_ELCON_RX, 0, true }
    };

    if ((status = ConfigCanFilters(ctx->hw.can, masks, sizeof(masks)/sizeof(masks[0]))) != 0)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    // Start CAN
    if ((status = HAL_CAN_Start(ctx->hw.can)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    // Enable interrupts
    if ((status = HAL_CAN_ActivateNotification(ctx->hw.can,
                                            CAN_IT_RX_FIFO0_MSG_PENDING |
                                            CAN_IT_RX_FIFO1_MSG_PENDING)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    // Configure TX header (example)
    ctx->hw.can_tx_header.StdId = 0x500;
    ctx->hw.can_tx_header.IDE = CAN_ID_STD;
    ctx->hw.can_tx_header.RTR = CAN_RTR_DATA;
    ctx->hw.can_tx_header.DLC = 8;
    ctx->hw.can_tx_header.TransmitGlobalTime = DISABLE;

    return status;
}


int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8])
{
    CAN_TxHeaderTypeDef* hdr = &ctx->hw.can_tx_header;

    // Determine if extended or standard ID
    if (id > CAN_STD_ID_MASK)
    {
        hdr->IDE = CAN_ID_EXT;
        hdr->ExtId = id & CAN_EXT_ID_MASK;
    }
    else
    {
        hdr->IDE = CAN_ID_STD;
        hdr->StdId = id & CAN_STD_ID_MASK;      
    }

    hdr->RTR = CAN_RTR_DATA;
    hdr->DLC = 8; // or customize if you have variable payloads
    hdr->TransmitGlobalTime = DISABLE;

    // Wait for a free mailbox
    uint32_t waited = 0;
    while (HAL_CAN_GetTxMailboxesFreeLevel(ctx->hw.can) == 0U)
    {
        if (waited >= CAN_TX_TIMEOUT_US)
        {
            ctx->stats.n_tx_can_drop_timeout++;
            return HAL_TIMEOUT;
        }
        DelayUs(ctx, CAN_TX_WAIT_US);
        waited += CAN_TX_WAIT_US;
    }

    int32_t result = HAL_CAN_AddTxMessage(ctx->hw.can, hdr, data, &ctx->hw.can_tx_mailbox);

    if (result != HAL_OK)
    {
        ctx->stats.n_tx_can_fail++;
        // ctx->led_state = LED_COMM_ERROR;
        ctx->last_can_err = HAL_CAN_GetError(ctx->hw.can);
    }
    else
    {
        ctx->stats.n_tx_can_frames++;
    }

    return result;
}

