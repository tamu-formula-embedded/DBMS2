/** 
 * 
 * Distributed BMS      Stack Controller Module
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Justus Languell  <justus@tamu.edu>
 *                      Cam Stone        <cameron28202@tamu.edu>
 *                      Abhinav Akavaram <abhinav.akavaram@tamu.edu>
 *                      Eli Nicksic      <eli.n@tamu.edu>
 */
#include "stack.h"


/**
 * Sends a wake blip to the battery stack
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int SendStackWakeBlip(DbmsCtx* ctx)
{
    return SendStackBlip(ctx, 25700 / 2);
}
/**
 * Sends a shutdown blip to the battery stack
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int SendStackShutdownBlip(DbmsCtx* ctx)
{
    return SendStackBlip(ctx, 128000);
}

/**
 * @brief Wake the battery stack
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int StackWake(DbmsCtx* ctx)
{
    static uint8_t FRAME_WAKE_STACK[] = {0x90, 0x0, 0x03, 0x9, 0x20, 0x13, 0x95};

    int status = 0;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackWakeBlip(ctx)) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackFrame(ctx, FRAME_WAKE_STACK, sizeof(FRAME_WAKE_STACK))) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        HAL_Delay(15 + 12 * N_STACKDEVS); // wtf  -- microseconds dumbass
    }
    return status;
}


/**
 * @brief Shuts down the battery stack
 * Use this when we turn the vehicle off, or when going to sleep
 * We could also use this in response to a critical fault
 * 
 * @param ctx Context pointer 
 * @return Error code 
 */
int StackShutdown(DbmsCtx* ctx)
{
    static uint8_t FRAME_SHUTDOWN_STACK[] = {0xD0, 0x03, 0x9, (1 << 3), 0x00, 0x00};

    int status = 0;
    for (int i = 0; i < 2; i++)
    {
        if ((status = SendStackFrameSetCrc(ctx, FRAME_SHUTDOWN_STACK, sizeof(FRAME_SHUTDOWN_STACK))) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        if ((status = SendStackShutdownBlip(ctx)) != 0)
        {
            CAN_REPORT_FAULT(ctx, status);
            return status;
        }
        HAL_Delay(100);
    }
    return status;
}

void SendOtpEccDatain(DbmsCtx* ctx)
{
    uint8_t frame_otp_ecc_datain[] = {0xD0, 0x03, 0x43, 0x00, 0x00, 0x00};
    // uint8_t frame_otp_ecc_datain[] = { 0xD0, 0x03, 0x4C, 0x00, 0x00, 0x00 };
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(ctx, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}

void SendAutoAddr(DbmsCtx* ctx)
{
    // 0xB0 at first
    uint8_t frame_addr_dev[] = {0xD0, 0x03, 0x06, 0x00, 0x00, 0x00};
    for (int i = 0; i <= N_STACKDEVS; i++)
    {
        SendStackFrameSetCrc(ctx, frame_addr_dev, sizeof(frame_addr_dev));
        frame_addr_dev[3]++;
    }
}

void SendSetStackTop(DbmsCtx* ctx)
{
    // Sets all devices as stack devices
    uint8_t frame_set_stack_devices[] = {0xD0, 0x03, 0x08, 0x02, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_devices, sizeof(frame_set_stack_devices));

    // Sets bridge device as non-stack device and bottom of stack
    uint8_t frame_set_stack_base[] = {0x90, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_base, sizeof(frame_set_stack_base));

    // Sets top of stack
    uint8_t frame_set_stack_top[] = {0x90, N_STACKDEVS - 1, 0x03, 0x08, 0x03, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_set_stack_top, sizeof(frame_set_stack_top));
}

void ReadOtpEccDatain(DbmsCtx* ctx)
{
    // uint8_t frame_otp_ecc_datain[] = { 0xA0, 0x03, 0x43, 0x00, 0x00, 0x00};
    uint8_t frame_otp_ecc_datain[] = {0xC0, 0x03, 0x4C, 0x00, 0x00, 0x00};
    for (int i = 0; i < 8; i++)
    {
        SendStackFrameSetCrc(ctx, frame_otp_ecc_datain, sizeof(frame_otp_ecc_datain));
        frame_otp_ecc_datain[2]++;
    }
}


/**
 * @brief Full auto addressing procedure
 * 
 * @param ctx Context pointer 
 */
void StackAutoAddr(DbmsCtx* ctx)
{
    SendOtpEccDatain(ctx); // step 1

    static uint8_t FRAME_ENABLE_AUTO_ADDR[] = {0xD0, 0x03, 0x09, 0x01, 0x0F, 0x74};
    SendStackFrameSetCrc(ctx, FRAME_ENABLE_AUTO_ADDR, sizeof(FRAME_ENABLE_AUTO_ADDR)); // step 2

    SendAutoAddr(ctx); // step 3

    static uint8_t FRAME_SET_ALL_STACK[] = {0xD0, 0x03, 0x08, 0x02, 0x0, 0x0};
    SendStackFrameSetCrc(ctx, FRAME_SET_ALL_STACK, sizeof(FRAME_SET_ALL_STACK)); // step 4

    SendSetStackTop(ctx); // step 5

    ReadOtpEccDatain(ctx); // step 6

    // static uint8_t FRAME_READ_DIR0_ADDR[] = { 0xA0, 0x00, 0x03, 0x06, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_READ_DIR0_ADDR, sizeof(FRAME_READ_DIR0_ADDR));

    // static uint8_t FRAME_CONF1_READ[] = { 0x80, 0x00, 0x20, 0x01, 0x01, 0x00, 0x00 };
    // SendStackFrameSetCrc(ctx, FRAME_CONF1_READ, sizeof(FRAME_CONF1_READ));
}

/**
 * @brief Set the number of active cells in the stack
 * 
 * @param ctx Context pointer 
 * @param n_active_cells 
 */
void StackSetNumActiveCells(DbmsCtx* ctx, uint8_t n_active_cells)
{
    uint8_t frame_set_active_cell[] = {0xD0, 0x00, 0x03, n_active_cells, 0x00, 0x00};

    SendStackFrameSetCrc(ctx, frame_set_active_cell, sizeof(frame_set_active_cell));
}


/*****************************
 *  VOLTAGE/TEMP READINGS 
 *****************************/

/**
 * @brief Enable the voltage tap ADCs
 * 
 * @param ctx Context pointer 
 */
void StackSetupVoltReadings(DbmsCtx* ctx)
{
    static uint8_t FRAME_SET_ADC_CONT_RUN[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, FRAME_SET_ADC_CONT_RUN, sizeof(FRAME_SET_ADC_CONT_RUN));
}


/**
 * @brief Request and populate voltage readings for a monitor by side-addr
 * 
 * @param ctx Context pointer 
 * @param addr Index of the side to read for
 * 
 * @deprecated
 */
void StackUpdateVoltReadingSingle(DbmsCtx* ctx, uint16_t addr)
{
    int status = 0;
    static uint8_t rx_buffer_v[1024];
    uint8_t in_addr = addr * 2 + 1;
    uint8_t frame[] = {0x80, in_addr & 0xff, 0x05, 0x68 + 2 * (16 - N_GROUPS_PER_SIDE), N_GROUPS_PER_SIDE * 2 - 1, 0x00, 0x00};

    size_t data_size = N_GROUPS_PER_SIDE * sizeof(int16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size);

    if ((status = SendStackFrameSetCrc(ctx, frame, sizeof(frame))) != 0) { }
    // TODO: what the fuck is the +2 for?
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_v, expected_rx_size+2, STACK_RECV_TIMEOUT)) != 0) { } 
    ctx->stats.n_rx_stack_frames++;

    uint8_t* data = rx_buffer_v;
    while (data[0] != frame[3]) data++;
    data++;
    uint16_t f_crc = (data[data_size]) + (data[data_size+1] << 8);
    *(data-1) = frame[3];
    *(data-2) = frame[2];
    *(data-3) = in_addr;
    *(data-4) = frame[4];
    uint16_t c_crc = CalcCrc16(data-4, data_size+4);
    if (f_crc != c_crc) 
    {
        ctx->stats.n_rx_stack_bad_crcs++;
        return;
    }

    for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
    {
        uint16_t raw = (data[j * sizeof(int16_t)] << 8) + (data[j * sizeof(int16_t) + 1]);
        ctx->cell_states[addr].voltages[j] = (raw * STACK_V_UV_PER_BIT) / 1000.0; // floating mV
    }
}

/**
 * @brief Request and populate voltage readings for the whole stack
 * 
 * @param ctx Context pointer 
 */
void StackUpdateAllVoltReadings(DbmsCtx* ctx)
{
    int status = 0;
    static uint8_t rx_buffer_v[1024];

    uint8_t frame[] = {0xA0, 0x05, 0x68 + 2 * (16 - N_GROUPS_PER_SIDE), N_GROUPS_PER_SIDE * 2 - 1, 0x00, 0x00};

    size_t data_size = N_GROUPS_PER_SIDE * sizeof(int16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_MONITORS;

    if ((status = SendStackFrameSetCrc(ctx, frame, sizeof(frame))) != 0) { }
    // TODO: abstract away the RX path for stack
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_v, expected_rx_size, STACK_RECV_TIMEOUT)) != 0) { } 

    for (int i = 0; i < N_MONITORS; i++)
    {
        ctx->stats.n_rx_stack_frames++;
        uint8_t* data = rx_buffer_v + (i * RX_FRAME_SIZE(data_size));
        for (int j = 0; data[0] != frame[2] && j < 1024; j++)
        {
            data++;
        }
        data++;
        uint8_t addr = *(data-3);
        uint16_t f_crc = (data[data_size]) + (data[data_size+1] << 8);

        uint16_t c_crc = CalcCrc16(data - 4, data_size+4);
        // CanLog(ctx,"%X != %X", f_crc, c_crc);
        if (f_crc != c_crc) 
        {
            ctx->stats.n_rx_stack_bad_crcs++;
            // CanLog(ctx, "bad %d\n", i);
            continue;
        }
        
        for (size_t j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            if (addr % 2 == 0) break;
            uint16_t raw = (data[j * sizeof(int16_t)] << 8) + (data[j * sizeof(int16_t) + 1]);
            ctx->cell_states[addr / 2].voltages[j] = (raw * STACK_V_UV_PER_BIT) / 1000.0; // floating mV
            // if (ctx->cell_states[addr / 2].voltages[j]  > 4500)
            // {
            //     CanLog(ctx, "%d", i + 1);
            // }
        }
    }
}

/**
 * @brief Configure's for temp readings
 * 
 * @param ctx Context pointer 
 */
void StackSetupGpio(DbmsCtx* ctx)
{
    // Note 2: Also configures GPIO8 even though we dont need to, hence just setting GPIO8 to output low
    uint8_t frame_gpio_configs[] = {0xB3, 0x00, 0x0E, 0x09, 0x09, 0x09, 0x21, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_gpio_configs, sizeof(frame_gpio_configs));

    // Setting up TSREF to active
    uint8_t frame_tsref_setup[] = {0xB0, 0x03, 0x0A, 0x01, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_tsref_setup, sizeof(frame_tsref_setup));
    DelayUs(ctx, 10);
}

/**
 * @brief Configure the heartbeat timeout
 * 
 * @param ctx Context pointer 
 */
void StackConfigTimeout(DbmsCtx* ctx)
{
    uint8_t hbcfg_frame[] = {0xD0, 0x00, 0x19, 0x0A, 0xB3, 0x73};
    SendStackFrame(ctx, hbcfg_frame, sizeof(hbcfg_frame));          // crc already encoded 
    //SendStackFrameSetCrc(ctx, hbcfg_frame, sizeof(hbcfg_frame));

    DelayUs(ctx, 10);
}


void StackUpdateAllTempReadings(DbmsCtx* ctx)
{
    int status = 0;
    static uint8_t rx_buffer_t[1024];

    uint8_t frame2[] = {0xA0, 0x05, 0x92, 4 * 2 - 1, 0x00, 0x00};

    size_t data_size = 4 * sizeof(int16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_MONITORS;

    if ((status = SendStackFrameSetCrc(ctx, frame2, sizeof(frame2))) != 0) { }
    // TODO: redo the RX path for stack
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_t, expected_rx_size, STACK_RECV_TIMEOUT)) != 0) { } 
    for (int i = 0; i < N_MONITORS; i++)
    {
        ctx->stats.n_rx_stack_frames++;

        uint8_t* data = rx_buffer_t + (i * RX_FRAME_SIZE(data_size));
        while (data[0] != frame2[2]) data++;
        data++;
        uint16_t f_crc = (data[data_size]) + (data[data_size+1] << 8);
        *(data-1) = frame2[2];
        *(data-2) = frame2[1];
        *(data-3) = i + 1;
        *(data-4) = frame2[3];
        uint16_t c_crc = CalcCrc16(data-4, data_size+4);
        if (f_crc != c_crc) 
        {
            ctx->stats.n_rx_stack_bad_crcs++;
            continue;
        }
        uint8_t offset = ctx->mux_selector;
        uint8_t temps = offset == 0 ? 4 : 3;
        for (size_t j = 0; j < temps; j++)
        {
            uint16_t raw = (data[j * sizeof(int16_t)] << 8) + (data[j * sizeof(int16_t) + 1]);
            ctx->cell_states[i].temps[4 * j + offset] = ThermVoltToTemp(ctx, MAX(0, (raw * STACK_T_UV_PER_BIT) / 1000000.0));
        }
    }
}

/**
 * @brief Replace missing thermistor readings with the average of the valid cells
 * 
 * @param ctx Context pointer 
 */
void FillMissingTempReadings(DbmsCtx* ctx)
{
    int n = 0;
    float sum = 0;
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
            float t = ctx->cell_states[i].temps[j];
            // TODO: remove (3, 7) blacklisted
            if ((t >= 18 && t < 120.0f) || (i == 3 && j == 7))
            {
                n++;
                sum += t;
            }

        }
    }

    float avg = sum / n;
    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
             float t = ctx->cell_states[i].temps[j];
            if ((t < 18 || t >= 120.0f) || (i == 3 && j == 7))
            {
                ctx->cell_states[i].temps[j] = avg;
            }
        }
    }
}

/**
 * @brief Compute min, avg, max voltages and temperatures 
 * 
 * @param ctx Context pointer 
 */
void StackCalcStats(DbmsCtx* ctx)
{
    float v_min = 999999.0f, v_max = 0.0f, v_sum = 0.0f;
    float t_min = 999.0f,    t_max = 0.0f, t_sum = 0.0f;

    for (int i = 0; i < N_SIDES; i++)
    {
        for (int j = 0; j < N_GROUPS_PER_SIDE; j++)
        {
            float v = ctx->cell_states[i].voltages[j];
            v_min = MIN(v_min, v);
            v_max = MAX(v_max, v);
            v_sum += v;
        }

        for (int j = 0; j < N_TEMPS_PER_SIDE; j++)
        {
            float t = ctx->cell_states[i].temps[j];
            t_min = MIN(t_min, t);
            t_max = MAX(t_max, t);
            t_sum += t;
        }
    }

    ctx->stats.min_v = v_min / 1000.0f;
    ctx->stats.max_v = v_max / 1000.0f;
    ctx->stats.avg_v = v_sum / (N_SIDES * N_GROUPS_PER_SIDE) / 1000.0f;
    ctx->stats.pack_v = v_sum;

    ctx->stats.min_t = t_min;
    ctx->stats.max_t = t_max;
    ctx->stats.avg_t = t_sum / (N_SIDES * N_TEMPS_PER_SIDE);
}


/*****************************
 *   LED CONTROL
 *****************************/

// TODO: document + reeval confusing names

int ToggleMonitorChipLed(DbmsCtx* ctx, bool on, uint8_t dev_number)
{
    // Turns on or off LED connected to GPIO8 on the specified monitor chip
    int status = 0;
    uint8_t on_off_value = 0x5; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x4;
    uint8_t led_change_write_com[] = {0x90, dev_number, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send single device write command frame
    if ((status = SendStackFrameSetCrc(ctx, led_change_write_com, sizeof(led_change_write_com))) != 0)
    {
        return status;
    }
    return status;
}

int ToggleAllMonitorChipLeds(DbmsCtx* ctx, bool on)
{
    // Turns on or off LED connected to GPIO8 on all monitor chips
    // Note for future:
    int status = 0;
    uint8_t on_off_value = 0x5; // On = 0x20, Off = 0x28
    if (on) on_off_value = 0x4;
    uint8_t leds_change_write_com[] = {0xB0, 0x00, 0x11, on_off_value, 0x00, 0x00};

    // Send stack device write command frame
    if ((status = SendStackFrameSetCrc(ctx, leds_change_write_com, sizeof(leds_change_write_com))) != 0)
    {
        return status;
    }
    return status;
}

void MonitorLedBlink(DbmsCtx* ctx)
{
    //if (!ctx->flags.active) return;

    uint64_t curr_ts = GetUs(ctx) - ctx->timing.m_led_blink_ts;

    if (ctx->flags.m_led_on)
    {
        if (curr_ts > 100000)
        {
            ToggleAllMonitorChipLeds(ctx, false);
            ctx->flags.m_led_on = false;
            ctx->timing.m_led_blink_ts = GetUs(ctx);
        }
    }
    else
    {
        if (curr_ts > 1000000)
        {
            ToggleAllMonitorChipLeds(ctx, true);
            ctx->flags.m_led_on = true;
            ctx->timing.m_led_blink_ts = GetUs(ctx);
        }
    }
}

/*****************************
 *   FAULT READING
 *****************************/

// TODO: clean this one up when we look into this stuff more
//       there all very legacy

int SetFaultMasks(DbmsCtx* ctx)
{
    return 0;
}

int PollFaultSummary(DbmsCtx* ctx)
{
    // int status;
    // uint8_t dev_addr;
    // uint8_t response[25];
    // uint8_t sendframe[] = {0x80, dev_addr, 0x05, 0x2D, 0x00, 0x00, 0x00};

    // if ((status = SendStackFrameSetCrc(ctx, sendframe, sizeof(sendframe))) != 0)
    // {
    //     //who cares
    // }

    // if ((status = HAL_UART_Receive(ctx->hw.uart, response, RX_FRAME_SIZE(1), STACK_RECV_TIMEOUT)) != 0) { } 
    // ctx->stats.n_rx_stack_frames++;

    // ctx->bridge_fault_summary = response[4];
    return 0;
}

/*****************************
 *  BALANCING 
 *****************************/

void StackBalancingConfig(DbmsCtx* ctx)
{
    // Setting balancing method to auto balancing and to stop at fault
    // BAL_CTRL2_CONFIG = 0x31
    uint8_t frame_cb_config[] = {0xB0, 0x03, 0x2F, 0x31, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_cb_config, sizeof(frame_cb_config));
}

void StackSetDeviceBalanceTimers(DbmsCtx* ctx, uint8_t dev_addr, bool odds, StackBalanceTimes bal_time_idx)
{
    bool* cells_to_bal = ctx->cell_states[dev_addr].cells_to_balance;

    uint16_t base_reg = 0x0327;  // CB_CELL1_CTRL (starts at cell 1, goes down)
    uint8_t bal_time = MIN((uint8_t)bal_time_idx, (uint8_t)__N_BAL_TIMES);
    
    // Write each cell timer individually
    for (size_t i = 0; i < N_GROUPS_PER_SIDE; ++i)
    {
        uint16_t reg_addr = base_reg - i; // registers decrement
        uint8_t timer_val = i % 2 == odds && cells_to_bal[i] ? bal_time : 0x00;
        
        uint8_t frame[7];
        frame[0] = 0x90;                        // single device write, 1 byte
        frame[1] = dev_addr;                 // which monitor chip
        frame[2] = reg_addr >> 8;               // register MSB
        frame[3] = reg_addr & 0xFF;             // register LSB  
        frame[4] = timer_val;                   // timer value
        frame[5] = 0x00;                        // crc placeholder
        frame[6] = 0x00;                        // crc placeholder
        
        SendStackFrameSetCrc(ctx, frame, sizeof(frame));
        HAL_Delay(2);  // small delay between writes
    }
}

void StackStartDeviceBalancing(DbmsCtx* ctx, uint8_t dev_addr)
{
    // this is the final trigger - balancing doesn't start until bal_ctrl2 is set
    //#define BAL_CTRL2_BAL_GO_MASK 0x02
    //uint8_t bal_ctrl2_frame[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_BAL_GO_MASK, 0x00, 0x00};
    
    uint8_t frame[7];

    frame[0] = 0x90; // single stack write
    frame[1] = dev_addr;
    frame[2] = 0x03; // bal_ctrl2 reg
    frame[3] = 0x2F; // bal_ctrl2 reg 
    frame[4] = 0x02; // bal_go bit
    frame[5] = 0x00; // crc
    frame[6] = 0x00; // crc
    SendStackFrameSetCrc(ctx, frame, sizeof(frame));
    HAL_Delay(8);
}

void StackStartBalancing(DbmsCtx* ctx, bool odds, int32_t bal_time)
{
    for(size_t side = 0; side < N_SIDES; ++side)
    {
        uint8_t dev_addr = side * N_MONITORS_PER_SIDE + 1;
        StackSetDeviceBalanceTimers(ctx, dev_addr, odds, bal_time);
        StackStartDeviceBalancing(ctx, dev_addr);
    }
}

void StackComputeCellsToBalance(DbmsCtx* ctx, bool odds, int32_t threshold_mv)
{
    // if any segment needs balancing - if this is false at the end we can skip balancing
    float balance_threshold = ctx->charging.pre_bal_min_v + threshold_mv;
    CanLog(ctx, "minv = %d chbalth = %d\n", (int)ctx->charging.pre_bal_min_v, (int)balance_threshold);
    // if (ctx->stats.max_v > balance_threshold) return false;
    for (size_t side = 0; side < N_SIDES; side++)
    {
        for (size_t group = 0; group < N_GROUPS_PER_SIDE; group++)
        {
            float voltage = ctx->charging.pre_bal_average_v[side][group];
            ctx->cell_states[side].cells_to_balance[group] = voltage > balance_threshold && group % 2 == odds;
            CanLog(ctx, "%d ", (int) ctx->charging.pre_bal_average_v[side][group]);
        }   
        CanLog(ctx, "\n");
    }
}

bool StackNeedsToBalance(DbmsCtx* ctx, bool odds, int32_t threshold_mv)
{
    float balance_threshold = 1000 * ctx->stats.min_v + threshold_mv;
    for (size_t side = 0; side < N_SIDES; side++)
    {
        for (size_t group = 0; group < N_GROUPS_PER_SIDE; group++)
        {
            float voltage = ctx->charging.pre_bal_average_v[side][group];
            if(voltage > balance_threshold && group % 2 == odds) return true;
        }
    }
    return false;
}

void StackDumpCellsToBalance(DbmsCtx* ctx)
{
    for (size_t side_index = 0; side_index < N_SIDES; ++side_index)
    {
        CanLog(ctx, "Side ");
        // Convert side_index to string manually
        char side_str[8];
        side_str[0] = '0' + side_index;
        side_str[1] = ':';
        side_str[2] = ' ';
        side_str[3] = '\0';
        CanLog(ctx, side_str);
        
        bool found_any = false;
        for (size_t group = 0; group < N_GROUPS_PER_SIDE; ++group)
        {
            if (ctx->cell_states[side_index].cells_to_balance[group])
            {
                char cell_str[8];
                cell_str[0] = 'C';
                cell_str[1] = '0' + (group / 10);      // Tens digit
                cell_str[2] = '0' + (group % 10);      // Ones digit
                cell_str[3] = ' ';
                cell_str[4] = '\0';
                CanLog(ctx, cell_str);
                found_any = true;
            }
        }
        
        if (!found_any)
        {
            CanLog(ctx, "None");
        }
        
        CanLog(ctx, "\n");
    }
}


/**
 * @brief Request and populate voltage readings for a monitor by side-addr.
 * 
 * @param ctx Context pointer
 * @param addr Index of side to poll 
 */
void StackReadBalStat(DbmsCtx* ctx, uint16_t addr)
{
    int status = 0;
    static uint8_t rx_buffer[1024];
    uint8_t in_addr = addr * 2 + 1;
    uint8_t frame[] = {0x80, in_addr & 0xff, 0x05, 0x2B, 0, 0x00, 0x00};

    size_t expected_rx_size = RX_FRAME_SIZE(1);

    if ((status = SendStackFrameSetCrc(ctx, frame, sizeof(frame))) != 0) { }

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer, expected_rx_size+2, STACK_RECV_TIMEOUT)) != 0) { } 
    ctx->stats.n_rx_stack_frames++;

    // CanLog(ctx, "Frame:");
    // for (int i = 0; i < 12; i++)
    // {
    //     CanLog(ctx, "%02X ", rx_buffer[i]);
    // }
    // CanLog(ctx, "\n");

    uint8_t* data = rx_buffer;
    while (data[0] != frame[3]) data++;
    data++;
    uint16_t f_crc = (data[1]) + (data[1+1] << 8);
    *(data-1) = frame[3];
    *(data-2) = frame[2];
    *(data-3) = in_addr;
    *(data-4) = frame[4]; 

    // CanLog(ctx, "CRC = %04X\n", f_crc); 
    uint16_t c_crc = CalcCrc16(data-4, 1+4);
    if (f_crc != c_crc) 
    {
        // CanLog(ctx, "CRC! %X %X\n", c_crc, f_crc);
        ctx->stats.n_rx_stack_bad_crcs++;
        return;
    }

    CanLog(ctx, "BAL Stat = %X\n", data[0]);
}

/*****************************
 *  ANALOG MUX CONTROL
 *****************************/

/**
 * @brief Set the active channel on the analog multiplexers
 * 
 * Each monitor chip has two 4:1 analog multiplexers (Mux A and Mux B) that expand
 * the number of thermistors that can be read. The mux select lines are controlled
 * via GPIO pins on the monitor ics.
 * 
 * GPIO Configuration (register 0x0E):
 * - Bits [1:0]: Mux A select (S0, S1)
 * - Bits [3:2]: Mux B select (S0, S1) 
 * - Bits [7:4]: Other GPIO config (kept constant)
 * 
 * @param ctx Context pointer
 * @param dev_number Monitor chip device address
 * @param channel Mux channel to select (0-3)
 * @return 0 on success, -1 on invalid channel
 */
int SetMuxChannels(DbmsCtx* ctx, uint8_t channel)
{
    uint8_t gpio_value;

    switch(channel)
    {
        case 0:
            gpio_value = 0x2D; // 0b00101101 - Channel 0: S1=0, S0=1 for both muxes
            break;
        case 1:
            gpio_value = 0x2C; // 0b00101100 - Channel 1: S1=0, S0=0 for both muxes
            break;
        case 2:
            gpio_value = 0x25; // 0b00100101 - Channel 2: S1=1, S0=1 for both muxes
            break;
        case 3:
            gpio_value = 0x24; // 0b00100100 - Channel 3: S1=1, S0=0 for both muxes
            break;
        default:
            return -1;
    }
    
    uint8_t mux_select_cmd[] = {0xB0, 0x00, 0x0E, gpio_value, 0x00, 0x00};
    ctx->mux_selector = channel;
    return SendStackFrameSetCrc(ctx, mux_select_cmd, sizeof(mux_select_cmd));
}

/**
 * @brief Read a pair of GPIO voltage measurements and convert to temperature
 * 
 * Reads two consecutive 16-bit ADC registers from the monitor chip, converts to temps
 * 
 * @param ctx Context pointer
 * @param dev_number Monitor chip device address
 * @param start_reg Starting register address (reads 4 bytes: 2 registers × 16 bits)
 * @param temp_out1 Output pointer for first temperature (°C)
 * @param temp_out2 Output pointer for second temperature (°C)
 * @return Error code (0 on success, -1 on null pointer, HAL error otherwise)
 */
int ReadMuxGpioPair(DbmsCtx* ctx, uint8_t dev_number, uint16_t start_reg, float* temp_out1, float* temp_out2)
{
    int status = 0;
    uint8_t data[10];

    if (!temp_out1 || !temp_out2)
    {
        return -1;
    }

    uint8_t read_cmd[] = {0x80, dev_number, start_reg >> 8, start_reg & 0xFF, 0x03, 0x00, 0x00};
    if ((status = SendStackFrameSetCrc(ctx, read_cmd, sizeof(read_cmd))) != 0)
    {
        return status;
    }
    
    if ((status = HAL_UART_Receive(ctx->hw.uart, data, sizeof(data), STACK_RECV_TIMEOUT)) != 0)
    {
        return status;
    }
    ctx->stats.n_rx_stack_frames++;

    uint16_t raw_adc1 = (data[4] << 8) | data[5];
    uint16_t raw_adc2 = (data[6] << 8) | data[7];
    
    float voltage1 = (raw_adc1 * STACK_T_UV_PER_BIT) / 1000000.0;
    float voltage2 = (raw_adc2 * STACK_T_UV_PER_BIT) / 1000000.0;
    
    *temp_out1 = ThermVoltToTemp(ctx, MAX(0, voltage1));
    *temp_out2 = ThermVoltToTemp(ctx, MAX(0, voltage2));

    return status;
}

/**
 * @brief Read all four mux-connected thermistor temperatures for the current channel
 * 
 * Each monitor chip has 2 analog muxes (A and B), each with 2 output lines routed to
 * GPIO pins. This reads all 4 GPIO ADC values for the selected mux channel
 * 
 * Hardware mapping:
 * - GPIO3 (reg 0x592): Mux A, Output Y0
 * - GPIO4 (reg 0x594): Mux A, Output Y1  
 * - GPIO5 (reg 0x596): Mux B, Output Y0
 * - GPIO6 (reg 0x598): Mux B, Output Y1
 * 
 * must call setmuxchannel func first
 * 
 * @param ctx Context pointer
 * @param dev_number Monitor chip device address
 * @param gpio3 Output pointer for GPIO3 temperature (Mux A, Y0)
 * @param gpio4 Output pointer for GPIO4 temperature (Mux A, Y1)
 * @param gpio5 Output pointer for GPIO5 temperature (Mux B, Y0)
 * @param gpio6 Output pointer for GPIO6 temperature (Mux B, Y1)
 * @return Error code (0 on success, -1 on null pointer, HAL error otherwise)
 */
int ReadMuxOutputs4x1(DbmsCtx* ctx, uint8_t dev_number, float* gpio3, float* gpio4, float* gpio5, float* gpio6)
{
    int status = 0;

    if (!gpio3 || !gpio4 || !gpio5 || !gpio6)
    {
        return -1;
    }

    if ((status = ReadMuxGpioPair(ctx, dev_number, 0x592, gpio3, gpio4)) != 0)
        return status;
    if ((status = ReadMuxGpioPair(ctx, dev_number, 0x596, gpio5, gpio6)) != 0)
        return status;
    
    return status;
}