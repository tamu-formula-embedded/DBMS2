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
        CanTransmit(hw_ctx, CANID_CONSOLE_C0, chunk);
    }
}

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
                CanTransmit(hw, CANID_CELLSTATE_VOLTS, frame);
            }
            for (size_t k = 0; k < N_TEMPS; k++)
            {
                frame[2] = k;
                uint16_t t = ctx->cell_states[i][j].temps[k];
                frame[6] = (t & 0xff00) >> 8;
                frame[7] = (t & 0x00ff);
                CanTransmit(hw, CANID_CELLSTATE_TEMPS, frame);
            }
        }
    }
}

int CanReportFault(HwCtx* hw_ctx, char* fn, int line_num, int err_code)
{
    // TODO: add forward fn -> the last /

    static uint8_t buf[8] = {0};                        // only taking the first 8 characters
    static uint8_t frame[8] = {0};

    // Compression algorithm to encode up to 8 characters of the filename
    for (size_t i = 0; fn[i] != 0 && i <= 8; i++)       
    {
        if (isalpha(fn[i])) 
            buf[i] = tolower(fn[i]) - 'a';      // 0-25 = a-z
        if (fn[i] == '.') buf[i] = 26;          // 26 = 
        else buf[i] = 27;                       // 27   = other
        buf[i] &= 0b11111;                      // ensure 5 bits
    }

    // Pack the first 5 bytes (40 bits) with 8 x 5 bit character representations
    frame[0] = (buf[0] << 3) | (buf[1] >> 2);
    frame[1] = ((buf[1] & 0x03) << 6) | (buf[2] << 1) | (buf[3] >> 4);
    frame[2] = (buf[4] << 3) | (buf[5] >> 2);
    frame[3] = ((buf[5] & 0x03) << 6) | (buf[6] << 1) | (buf[7] >> 4);
    frame[4] = (buf[7] & 0x0F) << 4;

    frame[5] = (line_num >> 8) & 0xFF;
    frame[6] = line_num & 0x00ff;
    frame[7] = err_code & 0xff;
    
    CanTransmit(hw_ctx, CANID_FATAL_ERROR, frame);      // Ignore this error code

    return err_code;
}

int CanTxHeartbeat(HwCtx* hw, uint16_t settings_crc)
{
    static uint8_t frame[] = { N_SEGMENTS, N_MONITORS_PER_SEG, N_GROUPS, N_TEMPS, 0, 0, 0, 0};
    frame[6] = (settings_crc & 0xff00) >> 8;
    frame[7] = (settings_crc & 0xff);
    return CanTransmit(hw, CANID_TX_HEARTBEAT, frame);
}

#define UNPACK_UINT32(BUF)      (((BUF)[0] & 0xff000000) << 24)     \
                                | (((BUF)[1] & 0x00ff0000) << 16)   \
                                | (((BUF)[2] & 0x0000ff00) << 8)    \
                                | (((BUF)[3] & 0x000000ff)); 

int HandleCanConfig(HwCtx* hw, DbmsCtx* ctx, uint8_t* rx_data)
{
    // This is an inefficient frame format ):

    uint8_t cfg_id  = rx_data[0];
    int32_t cfg_val = UNPACK_UINT32(rx_data + 4);

    switch (cfg_id)
    {
        case CFGID_MAX_ALLOWED_PACK_VOLTS:
            ctx->settings.max_allowed_pack_voltage = cfg_val & 0xffff;
            break;
        case CFGID_QUIET_MS_BEFORE_SHUTDOWN:
            ctx->settings.quiet_ms_before_shutdown = cfg_val;
            break;

        default:
            break;
    }

    return 0;
}