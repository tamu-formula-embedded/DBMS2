/** 
 * 
 * Distributed BMS      CAN I/O Interface
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#include "can.h"
#include "context.h"

#define CAN_TX_QUEUE_SIZE 512

typedef struct 
{
    uint32_t id;        // up to 29-bit CAN ID
    uint32_t mask;      // 29-bit or 11-bit mask
    bool extended;      // true = extended (29-bit)
} CanFilterMask;

typedef struct {
    uint32_t id;
    uint8_t data[8];
    //uint8_t prio ??
} CanTxQueueItem;

typedef struct {
    CanTxQueueItem buffer[CAN_TX_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t count;
} CanTxQueue;

static CanTxQueue tx_queue = {0};

// cant change the interrupt function sigantures, store a global ctx ptr and set on init
static DbmsCtx* g_can_ctx = NULL;

/**
 * Configure CAN RX filters
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
    g_can_ctx = ctx;
    int status = 0;

    // Configure primary bus
    CanFilterMask masks_primary[] = 
    {
        { 0x0B0, 0x7F0, false },
        { 0x500, 0x700, false }
    };

    CanFilterMask masks_secondary[] = 
    {
        { CANID_ELCON_TX, 0, true },
        { CANID_ELCON_RX, 0, true }
    };


    if ((status = ConfigCanFilters(ctx->hw.can_primary, masks_primary, sizeof(masks_primary)/sizeof(masks_primary[0]))) != 0)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    if ((status = HAL_CAN_Start(ctx->hw.can_primary)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    if ((status = HAL_CAN_ActivateNotification(ctx->hw.can_primary,
                                            CAN_IT_RX_FIFO0_MSG_PENDING |
                                            CAN_IT_RX_FIFO1_MSG_PENDING |
                                            CAN_IT_TX_MAILBOX_EMPTY)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    // Configure secondary bus
    if ((status = ConfigCanFilters(ctx->hw.can_secondary, masks_secondary, sizeof(masks_secondary)/sizeof(masks_secondary[0]))) != 0)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    if ((status = HAL_CAN_Start(ctx->hw.can_secondary)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    if ((status = HAL_CAN_ActivateNotification(ctx->hw.can_secondary,
                                            CAN_IT_RX_FIFO0_MSG_PENDING |
                                            CAN_IT_RX_FIFO1_MSG_PENDING)) != HAL_OK)
    {
        ctx->led_state = LED_FIRMWARE_FAULT;
        return status;
    }

    // Initialize headers
    ctx->hw.can_hdr_primary.StdId = 0x500;
    ctx->hw.can_hdr_primary.IDE = CAN_ID_STD;
    ctx->hw.can_hdr_primary.RTR = CAN_RTR_DATA;
    ctx->hw.can_hdr_primary.DLC = 8;
    ctx->hw.can_hdr_primary.TransmitGlobalTime = DISABLE;

    ctx->hw.can_hdr_secondary.StdId = 0x500;
    ctx->hw.can_hdr_secondary.IDE = CAN_ID_STD;
    ctx->hw.can_hdr_secondary.RTR = CAN_RTR_DATA;
    ctx->hw.can_hdr_secondary.DLC = 8;
    ctx->hw.can_hdr_secondary.TransmitGlobalTime = DISABLE;

    return status;
}

static void CanSetHdrID(CAN_TxHeaderTypeDef* hdr, uint32_t id)
{
    if (!hdr) return;
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
}

static void SendFromQueue(CAN_HandleTypeDef *hcan)
{
    if (!g_can_ctx) return;
    CAN_TxHeaderTypeDef* hdr = &g_can_ctx->hw.can_hdr_primary;
    
    while (tx_queue.count > 0 && HAL_CAN_GetTxMailboxesFreeLevel(hcan) > 0)
    {
        CanTxQueueItem* entry = &tx_queue.buffer[tx_queue.tail];
        CanSetHdrID(hdr, entry->id);
        
        uint32_t mailbox;
        int32_t result = HAL_CAN_AddTxMessage(hcan, hdr, entry->data, &mailbox);

        if (result == HAL_OK)
        {
            tx_queue.tail = (tx_queue.tail + 1) % CAN_TX_QUEUE_SIZE;
            tx_queue.count--;

            g_can_ctx->stats.can_primary.n_tx_frames++;
        }
        else
        {
            g_can_ctx->stats.can_primary.n_tx_fail++;
            g_can_ctx->stats.can_primary.last_err = HAL_CAN_GetError(hcan);
            break;
        }
    }
}

int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8])
{
    CAN_TxHeaderTypeDef* hdr = &ctx->hw.can_hdr_primary;

    __disable_irq();
    
    if (tx_queue.count == 0 && HAL_CAN_GetTxMailboxesFreeLevel(ctx->hw.can_primary) > 0U)
    {
        CanSetHdrID(hdr, id);
        int32_t result = HAL_CAN_AddTxMessage(ctx->hw.can_primary, hdr, data, &ctx->stats.can_primary.tx_mailbox);

        if (result != HAL_OK)
        {
            ctx->stats.can_primary.n_tx_fail++;
            // ctx->led_state = LED_COMM_ERROR;
            ctx->stats.can_primary.last_err = HAL_CAN_GetError(ctx->hw.can_primary);
        }
        else
        {
            ctx->stats.can_primary.n_tx_frames++;
        }
        __enable_irq();
        return result;
    }
    
    if (tx_queue.count >= CAN_TX_QUEUE_SIZE)
    {
        tx_queue.tail = (tx_queue.tail + 1) % CAN_TX_QUEUE_SIZE;
        tx_queue.count--;
        ctx->stats.can_primary.n_tx_drop++;
    }

    tx_queue.buffer[tx_queue.head].id = id;
    memcpy(tx_queue.buffer[tx_queue.head].data, data, 8);
    tx_queue.head = (tx_queue.head + 1) % CAN_TX_QUEUE_SIZE;
    tx_queue.count++;
    ctx->stats.can_primary.n_tx_queued = tx_queue.count;
    
    __enable_irq();

    SendFromQueue(ctx->hw.can_primary);
    
    return HAL_OK;
}

/**
 * Transmit on the secondary (elcon) bus. No CAN Queue.
 */
int CanTransmitSecondary(DbmsCtx* ctx, uint32_t id, uint8_t data[8])
{
    CAN_TxHeaderTypeDef* hdr = &ctx->hw.can_hdr_secondary;

    // Wait for a free mailbox
    uint32_t waited = 0;
    while (HAL_CAN_GetTxMailboxesFreeLevel(ctx->hw.can_secondary) == 0U)
    {
        if (waited >= CAN_TX_TIMEOUT_US)
        {
            ctx->stats.can_secondary.n_tx_drop++;
            return HAL_TIMEOUT;
        }
        DelayUs(ctx, CAN_TX_WAIT_US);
        waited += CAN_TX_WAIT_US;
    }

    CanSetHdrID(hdr, id);
    int32_t result = HAL_CAN_AddTxMessage(ctx->hw.can_secondary, hdr, data, &ctx->stats.can_secondary.tx_mailbox);

    if (result != HAL_OK)
    {
        ctx->stats.can_secondary.n_tx_fail++;
        ctx->stats.can_secondary.last_err = HAL_CAN_GetError(ctx->hw.can_secondary);
    }
    else
    {
        ctx->stats.can_secondary.n_tx_frames++;
    }

    return result;
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance != CAN_PRIMARY_INST) return;
    SendFromQueue(hcan);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance != CAN_PRIMARY_INST) return;
    SendFromQueue(hcan);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance != CAN_PRIMARY_INST) return;
    SendFromQueue(hcan);
}
