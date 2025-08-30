//  
//  Copyright (c) Texas A&M University.
//  
#include "vehicle_interface.h"

int ConfigCan(DbmsCtx* ctx)
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
    if ((status = HAL_CAN_ConfigFilter(ctx->hw.can, &s_filter_cfg)) != HAL_OK)
    {
        ctx->led_state = LED_COMM_ERROR;
        return status;
    }
    if ((status = HAL_CAN_Start(ctx->hw.can)) != HAL_OK)
        {
        ctx->led_state = LED_COMM_ERROR;
        return status;
    }
    if ((status = HAL_CAN_ActivateNotification(ctx->hw.can, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING)) != HAL_OK) 
    {
        ctx->led_state = LED_COMM_ERROR;
        return status;
    }

    // Config TX header
    ctx->hw.can_tx_header.StdId = 0xdead;
    ctx->hw.can_tx_header.IDE = CAN_ID_STD;
    ctx->hw.can_tx_header.RTR = CAN_RTR_DATA;
    ctx->hw.can_tx_header.DLC = 8;
    ctx->hw.can_tx_header.TransmitGlobalTime = DISABLE;

    return status;
}



int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8])
{
    CAN_TxHeaderTypeDef *hdr = &ctx->hw.can_tx_header;

    hdr->StdId = id;      // ensure IDE=CAN_ID_STD elsewhere
    // hdr->IDE = CAN_ID_STD; hdr->RTR = CAN_RTR_DATA; hdr->DLC = 8; // set once at init if fixed

    // Wait (briefly) for a free mailbox instead of calling blindly.
    uint32_t waited = 0;
    while (HAL_CAN_GetTxMailboxesFreeLevel(ctx->hw.can) == 0U) {
        if (waited >= CAN_TX_TIMEOUT_US) {
            ctx->stats.n_tx_can_drop_timeout++;
            // ctx->led_state = LED_COMM_ERROR; // not worth a locking error 
            return HAL_TIMEOUT;
        }
        DelayUs(ctx, CAN_TX_WAIT_US);
        waited += CAN_TX_WAIT_US;
    }

    int32_t result = HAL_CAN_AddTxMessage(ctx->hw.can, hdr, data, &ctx->hw.can_tx_mailbox);

    if (result != HAL_OK) {
        ctx->stats.n_tx_can_fail++;
        ctx->led_state = LED_COMM_ERROR;
        ctx->last_can_err = HAL_CAN_GetError(ctx->hw.can); // capture why
    } else {
        ctx->stats.n_tx_can_frames++;
    }
    return result;
}

void CanLog(DbmsCtx* ctx, const char* fmt, ...) 
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
        CanTransmit(ctx, CANID_CONSOLE_C0, chunk);
        DelayUs(ctx, 10);
    }
}

#define PAD_BUFFER(SZ, K) (((SZ / K) + 1) * K)
#define PAD_BUFFER_3(SZ) (PAD_BUFFER(SZ, 3))

int SendCellVoltages(DbmsCtx* ctx)
{
   int status = 0;
    uint8_t  frame[8];
    uint16_t buffer[PAD_BUFFER_3(N_GROUPS_PER_SIDE)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++) 
        {
            #define CLAMP_U16(x) ((uint16_t)((x) < 0 ? 0 : ((x) > 65535 ? 65535 : (x))))
            buffer[j] = CLAMP_U16((long)lroundf(ctx->cell_states[i].voltages[j] * 10.0f));
            // CanLog(ctx, "v=%d mV -> s=%ld\n", (uint16_t)ctx->cell_states[i].voltages[j], s);
        }

        for (size_t j = 0; j < PAD_BUFFER_3(N_GROUPS_PER_SIDE); j += 3)
        {
            frame[0] = (uint8_t)i;        // monitor id
            frame[1] = (uint8_t)j;        // group index (in uint16_t units)

            memcpy_eswap2(frame + 2, buffer + j, 3 * sizeof(uint16_t));

            status = CanTransmit(ctx, CANID_CELLSTATE_VOLTS, frame);
            if (status != 0) return status;
        }
    }
    return 0; // todo: make a real return value
}

int SendCellTemps(DbmsCtx* ctx)
{
     int status = 0;
    uint8_t  frame[8];
    uint16_t buffer[PAD_BUFFER_3(N_TEMPS_PER_SIDE)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_TEMPS_PER_SIDE; j++) 
        {
            buffer[j] = ctx->cell_states[i].temps[j];   // TODO: add anti-conversion
        }

        for (size_t j = 0; j < PAD_BUFFER_3(N_TEMPS_PER_SIDE); j += 3)
        {
            frame[0] = (uint8_t)i;        // monitor id
            frame[1] = (uint8_t)j;        // group index (in uint16_t units)

            memcpy_eswap2(frame + 2, buffer + j, 3 * sizeof(uint16_t));

            status = CanTransmit(ctx, CANID_CELLSTATE_TEMPS, frame);
            if (status != 0) return status;
        }
    }
    return 0;
}

void FillShortModuleName(char* buffer, size_t sz, char* raw)
{
    char *sk, *head, *dot;
    for (sk = head = raw; *sk != 0; sk++)
    {
        if (*sk == '/' || *sk == '\\') head = sk+1;
    }
    size_t len = MIN(sz, sk-head);
    memcpy(buffer, head, len);
    if ((dot = memchr(buffer, '.', len)))
    {
        memset(dot, 0, (buffer+len)-dot);
    }
}
// todo: implement^

int CanReportFault(DbmsCtx* ctx, char* fn, int line_num, int err_code)
{
    // printf("%s %d\n", fn, line_num);
    ctx->led_state = LED_ERROR; // todo: why?

    // TODO: add forward fn -> the last /

    static uint8_t frame[8] = {0};

    // static uint8_t buf[8] = {0};                        // only taking the first 8 characters
    // Compression algorithm to encode up to 8 characters of the filename
    // for (size_t i = 0; fn[i] != 0 && i <= 8; i++)       
    // {
    //     if (isalpha((uint8_t)fn[i])) 
    //         buf[i] = tolower(fn[i]) - 'a';      // 0-25 = a-z
    //     if (fn[i] == '.') buf[i] = 26;          // 26 = 
    //     else buf[i] = 27;                       // 27   = other
    //     buf[i] &= 0b11111;                      // ensure 5 bits
    // }
    // Pack the first 5 bytes (40 bits) with 8 x 5 bit character representations
    // frame[0] = (buf[0] << 3) | (buf[1] >> 2);
    // frame[1] = ((buf[1] & 0x03) << 6) | (buf[2] << 1) | (buf[3] >> 4);
    // frame[2] = (buf[4] << 3) | (buf[5] >> 2);
    // frame[3] = ((buf[5] & 0x03) << 6) | (buf[6] << 1) | (buf[7] >> 4);
    // frame[4] = (buf[7] & 0x0F) << 4;
    // for (size_t i = 0; fn[i] != 0 && i <= 4; i++)       
    // {
    //     frame[i] = fn[i];
    // }

    // CanLog(ctx, "Errno %d @ %s:%d\n", err_code, fn, line_num);
    FillShortModuleName((char*)frame, 5, fn);

    frame[5] = (line_num >> 8) & 0xff;
    frame[6] = line_num & 0x00ff;
    frame[7] = err_code & 0xff;

    CanTransmit(ctx, CANID_FATAL_ERROR, frame);      // Ignore this error code

    return err_code;
}

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc)
{
    // TODO: rethink this
    static uint8_t frame[] = { N_SEGMENTS, N_SIDES_PER_SEG, N_GROUPS_PER_SIDE, N_TEMPS_PER_SIDE, 0, 0, 0, 0};
    frame[6] = (settings_crc & 0xff00) >> 8;
    frame[7] = (settings_crc & 0xff);
    return CanTransmit(ctx, CANID_TX_HEARTBEAT, frame);
}

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action)
{
    // This is an inefficient frame format ):
    uint8_t cfg_id  = rx_data[0];
    int32_t cfg_set = 0, cfg_get = 0;
    
    cfg_set |= (rx_data[4] << 3*8);
    cfg_set |= (rx_data[5] << 2*8);
    cfg_set |= (rx_data[6] << 1*8);
    cfg_set |= (rx_data[7] << 0*8);
    // CanLog(ctx, "%d config_id\n", cfg_id);

#ifdef ACK_CFG
    uint8_t ack_frame[] = { action, cfg_id, 0, 0, 0, 0, 0, 0 };
    CanTransmit(ctx, CANID_TX_CFG_ACK, ack_frame);
#endif

//    CanLog(ctx, "CFG %d\n", cfg_id);
    if (!IsValidSettingIndex(ctx, cfg_id)) 
        return ERR_CFGID_NOT_FOUND;

    if (action == CFG_SET)
    {
        SetSetting(ctx, cfg_id, cfg_set);
        ctx->need_to_sync_settings = true;
    }
    else 
    {      
        cfg_get = GetSetting(ctx, cfg_id);
        uint8_t frame[] = { cfg_id, 0, 0, 0, 0, 0, 0, 0 };
        frame[4] = ((cfg_get >> 3*8) & 0xFF);
        frame[5] = ((cfg_get >> 2*8) & 0xFF);
        frame[6] = ((cfg_get >> 1*8) & 0xFF);
        frame[7] = ((cfg_get >> 0*8) & 0xFF);
        CanTransmit(ctx, CANID_TX_GET_CONFIG, frame);
    }

    return 0;
}

int SendMetric(DbmsCtx* ctx, uint8_t id, uint32_t value)
{
    static uint8_t data[8] = {0};
    data[0] = id;
    data[4] = ((value >> 3*8) & 0xFF);
    data[5] = ((value >> 2*8) & 0xFF);
    data[6] = ((value >> 1*8) & 0xFF);
    data[7] = ((value >> 0*8) & 0xFF);
    return CanTransmit(ctx, CANID_METRIC, data);
}

int SendMetrics(DbmsCtx* ctx)
{

    SendMetric(ctx, 0, ctx->stats.iters);

    SendMetric(ctx, 1, ctx->stats.n_tx_can_frames);
    SendMetric(ctx, 2, ctx->stats.n_rx_can_frames);
    SendMetric(ctx, 3, ctx->stats.n_unmatched_can_frames);

    SendMetric(ctx, 4, ctx->isense.current_ma);
    SendMetric(ctx, 5, ctx->isense.voltage1_mv);

    // SendMetric(ctx, 6, 0);
    // SendMetric(ctx, 7, 0);
    SendMetric(ctx, 8, ctx->isense.power_w);
    SendMetric(ctx, 9, ctx->isense.charge_as);
    SendMetric(ctx, 10, ctx->isense.energy_wh);

    SendMetric(ctx, 11, ctx->stats.n_tx_can_fail);
    SendMetric(ctx, 12, ctx->stats.n_tx_can_drop_timeout);

    SendMetric(ctx, 13, ctx->stats.looptime);
    SendMetric(ctx, 14, ctx->stats.end_delay);

    SendMetric(ctx, 15, ctx->faults.controller_mask);
    
    SendMetric(ctx, 16, ctx->stats.n_rx_stack_frames);
    SendMetric(ctx, 17, ctx->stats.n_rx_stack_bad_crcs);

    SendMetric(ctx, 18, ctx->stats.n_eeprom_writes);

    // TODO: perm sol.
    // SendMetric(ctx, 18, ctx->faults.monitor_masks[0]);

    return 0;
}

void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time)
{
    static uint8_t frame_set_stop_mode[8] = {0x34, 1, 0, 0, 0, 0, 0, 0};
    static uint8_t frame_set_run_mode[8] = {0x34, 1, 1, 0, 0, 0, 0, 0};
    static uint8_t frame_set_metric_cycle[8] = {0x20, 2, 0, 0, 0, 0, 0, 0};
    frame_set_metric_cycle[2] = (cycle_time & 0xFF00) >> 8;
    frame_set_metric_cycle[3] = (cycle_time & 0x00FF) >> 0;

    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_stop_mode);
    HAL_Delay(1);

    frame_set_metric_cycle[0] = 0x21;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x22;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x25;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x26;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);
    frame_set_metric_cycle[0] = 0x27;
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_metric_cycle);

    HAL_Delay(1);
    CanTransmit(ctx, CANID_ISENSE_COMMAND, frame_set_run_mode);
    HAL_Delay(1);
}

int64_t UnpackCurrentSensorData(uint8_t *data)  // expects >= 6 bytes, big-endian
{
    // Assemble into the low 48 bits
    uint64_t v = 0;
    v |= ((uint64_t)data[0] << 40);
    v |= ((uint64_t)data[1] << 32);
    v |= ((uint64_t)data[2] << 24);
    v |= ((uint64_t)data[3] << 16);
    v |= ((uint64_t)data[4] <<  8);
    v |= ((uint64_t)data[5] <<  0);

    // Sign bit is bit 47 (counting from 0). If set, extend the sign.
    if (v & (1ULL << 47)) {
        v |= ~((1ULL << 48) - 1);   // set all upper bits to 1
    }
    return (int64_t)v;
}
