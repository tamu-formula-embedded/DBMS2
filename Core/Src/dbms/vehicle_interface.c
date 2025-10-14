//
//  Copyright (c) Texas A&M University.
//
#include "vehicle_interface.h"


#include "main.h"
#include <string.h>
#include <stdbool.h>

typedef struct {
    uint16_t id;    // standard 11-bit CAN ID
    uint16_t mask;  // standard 11-bit mask
} CanFilterMask;

int ConfigCanFilters(CAN_HandleTypeDef *hcan, const CanFilterMask *filters, size_t count)
{
    const int base_bank = 14, ids_per_bank = 2;     
    HAL_StatusTypeDef status = HAL_OK;
    int banks_needed = (count + ids_per_bank - 1) / ids_per_bank;
    bool has_extra = (count % ids_per_bank) != 0;
    size_t idx = 0;

    // Configure all full banks (2 filters per bank)
    for (int bank = 0; bank < banks_needed - has_extra; bank++)
    {
        CAN_FilterTypeDef f = {0};
        f.FilterBank = base_bank + bank;
        f.FilterMode = CAN_FILTERMODE_IDMASK;     // use IDMASK for (id, mask)
        f.FilterScale = CAN_FILTERSCALE_16BIT;    // 16-bit allows 2 filters per bank
        f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        f.FilterActivation = ENABLE;
        f.SlaveStartFilterBank = base_bank;       // for dual-CAN setups

        // First pair
        f.FilterIdHigh     = filters[idx].id   << 5;
        f.FilterMaskIdHigh = filters[idx].mask << 5;
        idx++;
        // Second pair
        f.FilterIdLow      = filters[idx].id   << 5;
        f.FilterMaskIdLow  = filters[idx].mask << 5;
        idx++;

        status = HAL_CAN_ConfigFilter(hcan, &f);
        if (status != HAL_OK)
            return status;
    }
    // Handle leftover filter if odd count
    if (has_extra)
    {
        CAN_FilterTypeDef f = {0};
        f.FilterBank = base_bank + banks_needed - 1;
        f.FilterMode = CAN_FILTERMODE_IDMASK;
        f.FilterScale = CAN_FILTERSCALE_16BIT;
        f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
        f.FilterActivation = ENABLE;
        f.SlaveStartFilterBank = base_bank;

        // Fill the one remaining slot
        f.FilterIdHigh     = filters[idx].id   << 5;
        f.FilterMaskIdHigh = filters[idx].mask << 5;
        f.FilterIdLow      = 0x0000;  // unused
        f.FilterMaskIdLow  = 0x0000;  // unused

        status = HAL_CAN_ConfigFilter(hcan, &f);
        if (status != HAL_OK)
            return status;
    }

    return HAL_OK;
}

int ConfigCan(DbmsCtx* ctx)
{
    int status = 0;
    CanFilterMask masks[] = {
        { 0x500, 0x700 } // exact 0x321
        // { 0x540, 0x7FF }, // exact 0x540
        // { 0x550, 0x7F0 }, // range 0x550–0x55F
    };

    if ((status = ConfigCanFilters(ctx->hw.can, masks, sizeof(masks)/sizeof(masks[0]))) != 0)
    {
        ctx->led_state = LED_COMM_ERROR;
        return status;
    }

    // Start CAN
    if ((status = HAL_CAN_Start(ctx->hw.can)) != HAL_OK)
    {
        ctx->led_state = LED_COMM_ERROR;
        return status;
    }

    // Enable interrupts
    if ((status = HAL_CAN_ActivateNotification(ctx->hw.can,
                                            CAN_IT_RX_FIFO0_MSG_PENDING |
                                            CAN_IT_RX_FIFO1_MSG_PENDING)) != HAL_OK)
    {
        ctx->led_state = LED_COMM_ERROR;
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

int ConfigPwmLines(DbmsCtx* ctx)
{
    HAL_TIM_PWM_Start(ctx->hw.timer_pwm_1, TIM_CHANNEL_4);
    return 0;
}

int SetPwmStates(DbmsCtx* ctx)
{
    float soc = ctx->model.z_oc;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(ctx->hw.timer_pwm_1);
    uint32_t ccr = (uint32_t)((soc / 100.0f) * (arr + 1));

    __HAL_TIM_SET_COMPARE(ctx->hw.timer_pwm_1, TIM_CHANNEL_4, ccr);

    return 0;
}

int CanTransmit(DbmsCtx* ctx, uint32_t id, uint8_t data[8])
{
    CAN_TxHeaderTypeDef* hdr = &ctx->hw.can_tx_header;

    hdr->StdId = id; // ensure IDE=CAN_ID_STD elsewhere
    // hdr->IDE = CAN_ID_STD; hdr->RTR = CAN_RTR_DATA; hdr->DLC = 8; // set once at init if fixed

    // Wait (briefly) for a free mailbox instead of calling blindly.
    uint32_t waited = 0;
    while (HAL_CAN_GetTxMailboxesFreeLevel(ctx->hw.can) == 0U)
    {
        if (waited >= CAN_TX_TIMEOUT_US)
        {
            ctx->stats.n_tx_can_drop_timeout++;
            // ctx->led_state = LED_COMM_ERROR; // not worth a locking error
            return HAL_TIMEOUT;
        }
        DelayUs(ctx, CAN_TX_WAIT_US);
        waited += CAN_TX_WAIT_US;
    }

    int32_t result = HAL_CAN_AddTxMessage(ctx->hw.can, hdr, data, &ctx->hw.can_tx_mailbox);

    if (result != HAL_OK)
    {
        ctx->stats.n_tx_can_fail++;
        ctx->led_state = LED_COMM_ERROR;
        ctx->last_can_err = HAL_CAN_GetError(ctx->hw.can); // capture why
    }
    else
    {
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
    uint8_t frame[8];
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
            frame[0] = (uint8_t)i; // monitor id
            frame[1] = (uint8_t)j; // group index (in uint16_t units)

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
    uint8_t frame[8];
    uint16_t buffer[PAD_BUFFER_3(N_TEMPS_PER_SIDE)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
// #define CLAMP_U16(x) ((uint16_t)((x) < 0 ? 0 : ((x) > 65535 ? 65535 : (x))))
            buffer[j] = CLAMP_U16((long)lroundf(ctx->cell_states[i].temps[j] * 1000.0f));
        }

        for (size_t j = 0; j < PAD_BUFFER_3(N_TEMPS_PER_SIDE); j += 3)
        {
            frame[0] = (uint8_t)i; // monitor id
            frame[1] = (uint8_t)j; // group index (in uint16_t units)

            memcpy_eswap2(frame + 2, buffer + j, 3 * sizeof(uint16_t)); // <-- fixed

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
        if (*sk == '/' || *sk == '\\') head = sk + 1;
    }
    size_t len = MIN(sz, sk - head);
    memcpy(buffer, head, len);
    if ((dot = memchr(buffer, '.', len)))
    {
        memset(dot, 0, (buffer + len) - dot);
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

    CanTransmit(ctx, CANID_FATAL_ERROR, frame); // Ignore this error code

    return err_code;
}

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc)
{
    // TODO: rethink this
    static uint8_t frame[] = {N_SEGMENTS, N_SIDES_PER_SEG, N_GROUPS_PER_SIDE, N_TEMPS_PER_SIDE, 0, 0, 0, 0};
    frame[6] = (settings_crc & 0xff00) >> 8;
    frame[7] = (settings_crc & 0xff);
    return CanTransmit(ctx, CANID_TX_HEARTBEAT, frame);
}

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action)
{
    // This is an inefficient frame format ):
    uint8_t cfg_id = rx_data[0];
    int32_t cfg_set = 0, cfg_get = 0;

    cfg_set |= (rx_data[4] << 3 * 8);
    cfg_set |= (rx_data[5] << 2 * 8);
    cfg_set |= (rx_data[6] << 1 * 8);
    cfg_set |= (rx_data[7] << 0 * 8);
    // CanLog(ctx, "%d config_id\n", cfg_id);

#ifdef ACK_CFG
    uint8_t ack_frame[] = {action, cfg_id, 0, 0, 0, 0, 0, 0};
    CanTransmit(ctx, CANID_TX_CFG_ACK, ack_frame);
#endif

    //    CanLog(ctx, "CFG %d\n", cfg_id);
    if (!IsValidSettingIndex(ctx, cfg_id)) return ERR_CFGID_NOT_FOUND;

    if (action == CFG_SET)
    {
        SetSetting(ctx, cfg_id, cfg_set);
        ctx->need_to_sync_settings = true;
    }
    else
    {
        cfg_get = GetSetting(ctx, cfg_id);
        uint8_t frame[] = {cfg_id, 0, 0, 0, 0, 0, 0, 0};
        frame[4] = ((cfg_get >> 3 * 8) & 0xFF);
        frame[5] = ((cfg_get >> 2 * 8) & 0xFF);
        frame[6] = ((cfg_get >> 1 * 8) & 0xFF);
        frame[7] = ((cfg_get >> 0 * 8) & 0xFF);
        CanTransmit(ctx, CANID_TX_GET_CONFIG, frame);
    }

    return 0;
}

int SendMetric(DbmsCtx* ctx, uint8_t id, uint32_t value)
{
    static uint8_t data[8] = {0};
    data[0] = id;
    data[4] = ((value >> 3 * 8) & 0xFF);
    data[5] = ((value >> 2 * 8) & 0xFF);
    data[6] = ((value >> 1 * 8) & 0xFF);
    data[7] = ((value >> 0 * 8) & 0xFF);
    return CanTransmit(ctx, CANID_METRIC, data);
}

#define F2I_K(F, K) ((int)(F * K))
#define F2I_RAW(F)  (*((int*)&F))

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
    SendMetric(ctx, 19, ctx->faults_crc);

    SendMetric(ctx, 20, F2I_K(ctx->qstats.initial, 1e6));
    SendMetric(ctx, 21, F2I_K(ctx->qstats.accumulated_loss, 1e6));
    SendMetric(ctx, 22, F2I_K(ctx->model.Q, 1e6));
    SendMetric(ctx, 23, F2I_K(ctx->model.z_oc, 1e6));
    SendMetric(ctx, 24, F2I_K(ctx->model.V_oc, 1e6));
    SendMetric(ctx, 25, F2I_K(ctx->model.R_oc, 1e6));
    SendMetric(ctx, 26, F2I_K(ctx->model.z_rc, 1e6));
    SendMetric(ctx, 27, F2I_K(ctx->model.R0, 1e6));
    SendMetric(ctx, 28, F2I_K(ctx->model.R1, 1e6));
    SendMetric(ctx, 29, F2I_K(ctx->model.R2, 1e6));
    SendMetric(ctx, 30, F2I_K(ctx->model.R_rc, 1e6));
    SendMetric(ctx, 31, F2I_K(ctx->model.R_cell, 1e6));
    SendMetric(ctx, 32, F2I_K(ctx->model.I_lim, 1e6));

    SendMetric(ctx, 33, ctx->qstats.initial_set_ts);
    SendMetric(ctx, 34, (uint32_t)(GetRealTime(ctx) / 1000));   // conv to S

    SendMetric(ctx, 35, F2I_K(ctx->stats.max_t, 1e3));
    SendMetric(ctx, 36, F2I_K(ctx->stats.min_t, 1e3));
    SendMetric(ctx, 37, F2I_K(ctx->stats.avg_t, 1e3));
    SendMetric(ctx, 38, F2I_K(ctx->stats.max_v, 1e4));
    SendMetric(ctx, 39, F2I_K(ctx->stats.min_v, 1e4));
    SendMetric(ctx, 40, F2I_K(ctx->stats.avg_v, 1e4));

    SendMetric(ctx, 41, F2I_K(ctx->qstats.historic_accumulated_loss, 1e6));
    // TODO: perm sol.
    // SendMetric(ctx, 18, ctx->faults.monitor_masks[0]);

    SendMetric(ctx, 42, ctx->pl_pulse_t);
    SendMetric(ctx, 43, ctx->pl_last_ok_ts);
    SendMetric(ctx, 44, ctx->max_current_ma);


    return 0;
}

void ConfigCurrentSensor(DbmsCtx* ctx, uint16_t cycle_time)
{
    static uint8_t frame_set_stop_mode[8] = {0x34, 0, 1, 0, 0, 0, 0, 0};
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

int64_t UnpackCurrentSensorData(uint8_t* data) // expects >= 6 bytes, big-endian
{
    // Assemble into the low 48 bits
    uint64_t v = 0;
    v |= ((uint64_t)data[0] << 40);
    v |= ((uint64_t)data[1] << 32);
    v |= ((uint64_t)data[2] << 24);
    v |= ((uint64_t)data[3] << 16);
    v |= ((uint64_t)data[4] << 8);
    v |= ((uint64_t)data[5] << 0);

    // Sign bit is bit 47 (counting from 0). If set, extend the sign.
    if (v & (1ULL << 47))
    {
        v |= ~((1ULL << 48) - 1); // set all upper bits to 1
    }
    return (int64_t)v;
}
