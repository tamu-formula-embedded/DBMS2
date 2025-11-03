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

// so that we aren't just doing v > min(v) to filter groups - small margin
#define BALANCE_MARGIN          5.0f

#define CB_CELL1_CTRL_REG       0x31A

#define TIMER_VALUE             0x02    // 30 seconds - page 157

#define CB_RUN_BIT_MASK         0x08
#define BAL_STAT_ABORT_MASK     0x04
#define BAL_CTRL2_BAL_GO_MASK   0x02
#define BAL_CTRL2_CONFIG        0x31    // auto_bal = 1, otcb_en = 1, flt_stop = 1

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
 *   3 step process -
 *   1. set up individual cell timers (write non-zero values to cb_cell_ctrl registers) - need to experiment with timer
 * values
 *   2. configure balancing method
 *   3. set additional controls (write to bal_ctrl2 register to configure auto or manual balancing)
 */
void StackStartBalancing(DbmsCtx* ctx, bool odds, int32_t bal_time)
{
    for(size_t side = 0; side < N_SIDES; ++side)
    {
        uint8_t dev_addr = (side * N_MONITORS_PER_SIDE) + 1;
        
        StackSetDeviceBalanceTimer(ctx, dev_addr, ctx->cell_states[side_index].cells_to_balance, odds, bal_time);
        StackStartDeviceBalancing(ctx, dev_addr);
    }
}

void StackBalancingConfig(DbmsCtx* ctx)
{
    // Setting balancing method to auto balancing and to stop at fault
    uint8_t frame_cb_config[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_CONFIG, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_cb_config, sizeof(frame_cb_config));
}

void StackCalcBalanceTimers(DbmsCtx* ctx, int32_t threshold_mv)
{
    // if any segment needs balancing - if this is false at the end we can skip balancing
    float balance_threshold = ctx->stats.min_v + threshold_mv;
    // if (ctx->stats.max_v > balance_threshold) return false;

    for (size_t side = 0; side < N_SIDES; side++)
    {
        for (size_t group = 0; group < N_GROUPS_PER_SIDE; group++)
        {
            float voltage = ctx->cell_states[side].voltages[group];
            ctx->cell_states[side].cells_to_balance[group] = voltage > balance_threshold;
        }   
    }
}

void StackSetDeviceBalanceTimer(DbmsCtx* ctx, uint8_t device_addr, 
        bool cells_to_balance[N_GROUPS_PER_SIDE], bool odds, int32_t bal_time)
{
    uint16_t base_reg = 0x0327;  // CB_CELL1_CTRL (starts at cell 1, goes down)
    
    // Write each cell timer individually
    for (size_t i = 0; i < N_GROUPS_PER_SIDE; ++i)
    {
        uint16_t reg_addr = base_reg - i; // registers decrement
        uint8_t timer_val = i % 2 == odds && cells_to_balance[i] ? bal_time : 0x00;
        
        uint8_t frame[7];
        frame[0] = 0x90;                        // single device write, 1 byte
        frame[1] = device_addr;                 // which monitor chip
        frame[2] = reg_addr >> 8;               // register MSB
        frame[3] = reg_addr & 0xFF;             // register LSB  
        frame[4] = timer_val;                   // timer value
        frame[5] = 0x00;                        // crc placeholder
        frame[6] = 0x00;                        // crc placeholder
        
        SendStackFrameSetCrc(ctx, frame, sizeof(frame));
        HAL_Delay(2);  // small delay between writes
    }
}


void StackStartDeviceBalancing(DbmsCtx* ctx, uint8_t device_addr)
{
    // this is the final trigger - balancing doesn't start until bal_ctrl2 is set
    //uint8_t bal_ctrl2_frame[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_BAL_GO_MASK, 0x00, 0x00};
    
    uint8_t frame[7];

    frame[0] = 0x90; // single stack write
    frame[1] = device_addr;
    frame[2] = 0x03; // bal_ctrl2 reg
    frame[3] = 0x2F; // bal_ctrl2 reg 
    frame[4] = 0x02; // bal_go bit
    frame[5] = 0x00; // crc
    frame[6] = 0x00; // crc
    SendStackFrameSetCrc(ctx, frame, sizeof(frame));
    HAL_Delay(8);
}


/**
 * Request and populate voltage readings for a monitor by side-addr
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