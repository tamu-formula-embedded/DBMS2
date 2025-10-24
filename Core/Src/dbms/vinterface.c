/** 
 * 
 * Distributed BMS      CAN-based Telemtery
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 */
#include "vinterface.h"

/*****************************
 *      HEARTBEAT
 *****************************/

int CanTxHeartbeat(DbmsCtx* ctx, uint16_t settings_crc)
{
    // TODO: rethink this
    static uint8_t frame[] = {N_SEGMENTS, N_SIDES_PER_SEG, N_GROUPS_PER_SIDE, N_TEMPS_PER_SIDE, 0, 0, 0, 0};
    frame[6] = (settings_crc & 0xff00) >> 8;
    frame[7] = (settings_crc & 0xff);
    return CanTransmit(ctx, CANID_TX_HEARTBEAT, frame);
}

/*****************************
 *      CONFIG
 *****************************/

int HandleCanConfig(DbmsCtx* ctx, uint8_t* rx_data, CanConfigAction action)
{
    // This is an inefficient frame format ):
    uint8_t cfg_id = rx_data[0];
    int32_t cfg_set = 0, cfg_get = 0;

    cfg_set |= (rx_data[4] << 3 * 8);
    cfg_set |= (rx_data[5] << 2 * 8);
    cfg_set |= (rx_data[6] << 1 * 8);
    cfg_set |= (rx_data[7] << 0 * 8);

#ifdef ACK_CFG
    if (action == CFG_SET)
        CanTransmit(ctx, CANID_TX_CFG_ACK, rx_data);
#endif

    if (!IsValidSettingIndex(ctx, cfg_id)) return ERR_CFGID_NOT_FOUND;

    if (action == CFG_SET)
    {
        CanLog(ctx, "CFG %d %d\n", cfg_id, cfg_set);
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

/*****************************
 *      METRICS
 *****************************/

int SendMetric(DbmsCtx* ctx, uint16_t id, uint64_t value)
{
    static uint8_t data[8] = {0};
#ifdef CAN_VERSION_2
    data[0] = (value >> 56) & 0xFF;
    data[1] = (value >> 48) & 0xFF;
    data[2] = (value >> 40) & 0xFF;
    data[3] = (value >> 32) & 0xFF;
    data[4] = (value >> 24) & 0xFF;
    data[5] = (value >> 16) & 0xFF;
    data[6] = (value >> 8)  & 0xFF;
    data[7] = (value >> 0)  & 0xFF;
    return CanTransmit(ctx, (CANID_METRIC & 0xFFFFF000) | (id & 0xFFF), data);
#else
    data[0] = id & 0xFF;
    data[4] = ((value >> 3 * 8) & 0xFF);
    data[5] = ((value >> 2 * 8) & 0xFF);
    data[6] = ((value >> 1 * 8) & 0xFF);
    data[7] = ((value >> 0 * 8) & 0xFF);
    return CanTransmit(ctx, CANID_METRIC, data);
#endif
}

#define F2I_K(F, K) ((int)(F * K))

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

    SendMetric(ctx, 42, ctx->pl_pulse_t);
    SendMetric(ctx, 43, ctx->pl_last_ok_ts);
    SendMetric(ctx, 44, ctx->max_current_ma);

    SendMetric(ctx, 45, ctx->elcon.heartbeat);
    SendMetric(ctx, 46, ctx->elcon.voltage_out);
    SendMetric(ctx, 47, ctx->elcon.current_out);
    SendMetric(ctx, 48, ctx->elcon.status_flags);
    SendMetric(ctx, 49, ctx->stats.fault_line_faulted);

    return 0;
}

void SendPlex32(DbmsCtx* ctx, uint32_t id, uint32_t m)
{
    uint8_t data[8] = {0};
    data[0] = (m >> 3*8) & 0xff;
    data[1] = (m >> 2*8) & 0xff;
    data[2] = (m >> 1*8) & 0xff;
    data[3] = (m >> 0*8) & 0xff;
    CanTransmit(ctx, id, data);
}

void SendPlexMetrics(DbmsCtx* ctx)
{
    uint8_t data[8] = {0};
    uint8_t soc = CLAMP((uint8_t)(ctx->model.z_oc * 100), 0, 100);
    data[0] = soc;
    CanTransmit(ctx, 0x16, data);

    SendPlex32(ctx, 0x17, ctx->faults.controller_mask);
    SendPlex32(ctx, 0x18, ctx->isense.current_ma / 1000);
    SendPlex32(ctx, 0x19, ctx->isense.ima.current_mavg_ma / 1000);
    SendPlex32(ctx, 0x1a, ctx->pl_pulse_t);
    SendPlex32(ctx, 0x1b, ctx->stats.avg_v * (N_SIDES * N_GROUPS_PER_SIDE));
    SendPlex32(ctx, 0x1c, ctx->stats.iters);

}


/*****************************
 *      LOGGING
 *****************************/


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
        int32_t id = CANID_CONSOLE_C0;
        uint8_t chunk[8] = {0}; // zero out to pad shorter chunks
        int chunk_size = (len - i >= 8) ? 8 : (len - i);
        memcpy(chunk, &buffer[i], chunk_size);
#ifdef CAN_VERSION_2
        id |= (ctx->stats.n_logging_frames & 0xFFF);
#endif
        CanTransmit(ctx, id, chunk);
#ifndef CAN_VERSION_2
        DelayUs(ctx, 10);
#endif

        ctx->stats.n_logging_frames++;
    }
}


/*****************************
 *      CELL DATA 
 *****************************/

#define PAD_BUFFER(SZ, K) (((SZ / K) + 1) * K)
#define CLAMP_U16(x) ((uint16_t)((x) < 0 ? 0 : ((x) > 65535 ? 65535 : (x))))

#ifdef CAN_VERSION_2

int SendCellDataBuffer(DbmsCtx* ctx, uint32_t id_base, uint16_t side_n, uint16_t* buffer, size_t len)
{
    int status = 0;
    uint8_t frame[8];
    for (size_t j = 0; j < len; j += 4)
    {
        uint32_t id = id_base | (side_n << 4) | (j / 4);
        memcpy_eswap2(frame, buffer + j, 4 * sizeof(uint16_t));
        if ((status = CanTransmit(ctx, id, frame)) != 0) return status;
    }
    return status;
}

int SendCellVoltages(DbmsCtx* ctx)
{
    uint16_t buffer[PAD_BUFFER(N_GROUPS_PER_SIDE, 3)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            buffer[j] = CLAMP_U16((long)lroundf(ctx->cell_states[i].voltages[j] * 10.0f));
        }

        SendCellDataBuffer(ctx, CANID_CELLSTATE_VOLTS, i, buffer, ARRAY_LEN(buffer));
    }
    return 0;   // dont really care if it fails or not
}

int SendCellTemps(DbmsCtx* ctx)
{
    uint16_t buffer[PAD_BUFFER(N_GROUPS_PER_SIDE, 3)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            buffer[j] = CLAMP_U16((long)lroundf(ctx->cell_states[i].temps[j] * 1000.0f));
        }
        SendCellDataBuffer(ctx, CANID_CELLSTATE_TEMPS, i, buffer, ARRAY_LEN(buffer));
    }
    return 0;   
}

#else

#define PAD_BUFFER_3(SZ) (PAD_BUFFER(SZ, 3))
/**
 * Send cell voltage data (version 1)
 */
int SendCellVoltages(DbmsCtx* ctx)
{
    int status = 0;
    uint8_t frame[8];
    uint16_t buffer[PAD_BUFFER_3(N_GROUPS_PER_SIDE)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
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

/**
 * Send cell temperature data (version 1)
 */
int SendCellTemps(DbmsCtx* ctx)
{
    int status = 0;
    uint8_t frame[8];
    uint16_t buffer[PAD_BUFFER_3(N_TEMPS_PER_SIDE)] = {0};

    for (size_t i = 0; i < N_SIDES; i++)
    {
        for (size_t j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
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

#endif