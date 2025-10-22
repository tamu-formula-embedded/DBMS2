/** 
 * 
 * Distributed BMS      Stack Module -> Stack Balancing
 *
 * Copyright (C) 2025   Texas A&M University
 * 
 *                      Cam Stone <cameron28202@tamu.edu>
 */
#include "stack.h"

// TODO: rework all this

// to determine if we need balancing
#define BALANCE_LIMIT 1000.0f

// so that we aren't just doing v > min(v) to filter groups - small margin
#define BALANCE_MARGIN 5.0f

#if 0

/*
static const uint16_t CB_CELL_CTRL_REGS[14] = {
    0x327,  // CB_CELL1_CTRL
    0x326,  // CB_CELL2_CTRL
    0x325,  // CB_CELL3_CTRL
    0x324,  // CB_CELL4_CTRL
    0x323,  // CB_CELL5_CTRL
    0x322,  // CB_CELL6_CTRL
    0x321,  // CB_CELL7_CTRL
    0x320,  // CB_CELL8_CTRL
    0x31F,  // CB_CELL9_CTRL
    0x31E,  // CB_CELL10_CTRL
    0x31D,  // CB_CELL11_CTRL
    0x31C,  // CB_CELL12_CTRL
    0x31B,  // CB_CELL13_CTRL
    0x31A   // CB_CELL14_CTRL
};
*/
#define CB_CELL1_CTRL_REG 0x31A

#define TIMER_VALUE 0x02 // 30 seconds - page 157

#define CB_RUN_BIT_MASK 0x08
#define BAL_STAT_ABORT_MASK 0x04
#define BAL_CTRL2_BAL_GO_MASK 0x02
#define BAL_CTRL2_CONFIG 0x31 // auto_bal = 1, otcb_en = 1, flt_stop = 1


void StackFindMinMaxVoltages(DbmsCtx* ctx, size_t segment, float* min_voltage, float* max_voltage)
{

    for (size_t side_in_seg = 0; side_in_seg < N_SIDES_PER_SEG; ++side_in_seg)
    {

        size_t side_index = segment * N_SIDES_PER_SEG + side_in_seg;
        for (size_t group = 0; group < N_GROUPS_PER_SIDE; ++group)
        {
            float voltage = ctx->cell_states[side_index].voltages[group];

            if (voltage <= 0.0f) continue;

            if (voltage < *min_voltage) *min_voltage = voltage;
            if (voltage > *max_voltage) *max_voltage = voltage;
        }
    }
}

/**
 *  max(v) - min(v) > balance_limit -> we need to balance
 */
bool StackNeedsBalancing(DbmsCtx* ctx)
{
    // if any segment needs balancing - if this is false at the end we can skip balancing
    bool needs_balancing = false;

    for (size_t segment = 0; segment < N_SEGMENTS; ++segment)
    {
        float min_voltage = 5000.0f;
        float max_voltage = 0.0f;
        StackFindMinMaxVoltages(ctx, segment, &min_voltage, &max_voltage);

        // segment needs balancing
        if (max_voltage - min_voltage > BALANCE_LIMIT)
        {
            needs_balancing = true;

            // determine cells we need to balance
            float balance_threshold = min_voltage + BALANCE_MARGIN;
            for (size_t side_in_seg = 0; side_in_seg < N_SIDES_PER_SEG; ++side_in_seg)
            {
                size_t side_index = segment * N_SIDES_PER_SEG + side_in_seg;
                for (size_t group = 0; group < N_GROUPS_PER_SIDE; ++group)
                {
                    float voltage = ctx->cell_states[side_index].voltages[group];
                    ctx->cell_states[side_index].cells_to_balance[group] = voltage > balance_threshold;
                }
            }
        }
    }

    return needs_balancing;
}

void StackSetCellBalanceTimer(DbmsCtx* ctx, bool cells_to_balance[N_GROUPS_PER_SIDE])
{
    // no need for lookup table because of contiguous register memory addresses
    uint16_t start_reg = CB_CELL1_CTRL_REG;
    uint8_t frame_size = N_GROUPS_PER_SIDE + 5;
    // uint8_t frame_size = 4 + N_GROUPS_PER_SIDE + 2;
    uint8_t cb_bulk_frame[frame_size]; // 20 bytes max

    cb_bulk_frame[0] = 0xBE - 1;
    cb_bulk_frame[1] = start_reg >> 8;
    cb_bulk_frame[2] = start_reg & 0xFF;

    for (size_t i = 0; i < N_GROUPS_PER_SIDE; ++i)
    {
        cb_bulk_frame[3 + i] = cells_to_balance[i] ? TIMER_VALUE : 0;
    }

    cb_bulk_frame[frame_size - 2] = 0x00;
    cb_bulk_frame[frame_size - 1] = 0x00;

    SendStackFrameSetCrc(ctx, cb_bulk_frame, frame_size);
    HAL_Delay(8);
}

void StackStartBalancing(DbmsCtx* ctx)
{
    // this is the final trigger - balancing doesn't start until bal_ctrl2 is set
    uint8_t bal_ctrl2_frame[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_BAL_GO_MASK, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, bal_ctrl2_frame, sizeof(bal_ctrl2_frame));
    HAL_Delay(8);
}

/**
 *   3 step process -
 *   1. set up individual cell timers (write non-zero values to cb_cell_ctrl registers) - need to experiment with timer
 * values
 *   2. configure balancing method
 *   3. set additional controls (write to bal_ctrl2 register to configure auto or manual balancing)
 */
void StackUpdateBalancing(DbmsCtx* ctx)
{
    for (size_t segment = 0; segment < N_SEGMENTS; ++segment)
    {
        for (size_t side_in_seg = 0; side_in_seg < N_SIDES_PER_SEG; ++side_in_seg)
        {
            size_t side_index = segment * N_SIDES_PER_SEG + side_in_seg;

            StackSetCellBalanceTimer(ctx, ctx->cell_states[side_index].cells_to_balance);
        }
    }
    // StackStartBalancing(ctx);
}

bool StackBalancingComplete(DbmsCtx* ctx)
{

    uint8_t bal_stat_frame[] = {0xA0, 0x05, 0x2B, 0x00, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, bal_stat_frame, sizeof(bal_stat_frame));

    uint8_t rx_buffer[RX_FRAME_SIZE(1) * N_MONITORS];

    if (HAL_UART_Receive(ctx->hw.uart, rx_buffer, sizeof(rx_buffer), STACK_RECV_TIMEOUT) != HAL_OK)
    {
        // throw error
        return false;
    }

    RxStackFrame rx_frames[N_MONITORS];
    FillStackFrames(rx_frames, rx_buffer, sizeof(rx_buffer), N_MONITORS);
    uint8_t bal_stat_value = 0;
    for (size_t i = 0; i < N_MONITORS; ++i)
    {
        bal_stat_value = rx_frames[i].data[0];

        if ((bal_stat_value & CB_RUN_BIT_MASK) != 0)
        {
            return false; // Still balancing
        }
    }

    return true;

    /*

        // read bal_stat register from each active monitor chip
        // return true when CB_RUN = 0 for all monitors that were balancing
        for(size_t segment = 0; segment < N_SEGMENTS; ++segment){
            for(size_t side_in_seg = 0; side_in_seg < N_SIDES_PER_SEG; ++side_in_seg){
                //size_t side_index = segment * N_SIDES_PER_SEG + side_in_seg;
                //uint8_t monitor_addr = (side_index * 2) + 1;

                // bal_stat register is 52B
                uint8_t bal_stat_frame[] = {0xA0, 0x05, 0x2B, 0x00, 0x00, 0x00};

                SendStackFrameSetCrc(ctx, bal_stat_frame, sizeof(bal_stat_frame));

                // recieve response
                uint8_t rx_buffer[RX_FRAME_SIZE(1)];
                if(HAL_UART_Receive(ctx->hw.uart, rx_buffer, sizeof(rx_buffer), STACK_RECV_TIMEOUT) != HAL_OK){
                    // throw error
                    return false;
                }

                RxStackFrame rx_frame;
                FillStackFrame(&rx_frame, rx_buffer, 1); // 1 byte of data expected

                // check if CB_RUN bit is set
                uint8_t bal_stat_value = rx_frame.data[0];
                bool cb_run = (bal_stat_value & CB_RUN_BIT_MASK) != 0;

                if(cb_run){
                    return false; // Still balancing
                }
            }
        }
        return true;

        */
}

bool StackBalancingAbortedByFault(DbmsCtx* ctx)
{
    // page 42
    uint8_t bal_stat_frame[] = {0xA0, 0x05, 0x2B, 0x00, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, bal_stat_frame, sizeof(bal_stat_frame));

    // recieve response
    uint8_t rx_buffer[RX_FRAME_SIZE(1) * N_MONITORS];
    if (HAL_UART_Receive(ctx->hw.uart, rx_buffer, sizeof(rx_buffer), STACK_RECV_TIMEOUT) != HAL_OK)
    {
        // throw error
        return true;
    }

    RxStackFrame rx_frames[N_MONITORS];
    FillStackFrames(rx_frames, rx_buffer, 1, N_MONITORS); // 1 byte of data expected

    // check if abort bit is set
    uint8_t bal_stat_value = 0;
    for (size_t monitor = 0; monitor < N_MONITORS; monitor++)
    { // TODO: Need to change this to iterate by side instead of by monitor
        bal_stat_value = rx_frames[monitor].data[0];
        if ((bal_stat_value & BAL_STAT_ABORT_MASK) != 0)
        {
            return true; // Balancing aborted
        }
    }

    return false;
}

void StackBalancingConfig(DbmsCtx* ctx)
{
    // Setting balancing method to auto balancing and to stop at fault
    uint8_t frame_cb_config[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_CONFIG, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_cb_config, sizeof(frame_cb_config));
}

void StackStartCharging(DbmsCtx* ctx)
{
    // begin charging if:
    // hearbeat from car not detected
    // heartbeat from charger detected
    // shutdown circuit active

    
}

#endif