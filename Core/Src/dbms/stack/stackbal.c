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
#define BALANCE_LIMIT           1000.0f

// so that we aren't just doing v > min(v) to filter groups - small margin
#define BALANCE_MARGIN          5.0f

#define CB_CELL1_CTRL_REG       0x31A

#define TIMER_VALUE             0x02    // 30 seconds - page 157

#define CB_RUN_BIT_MASK         0x08
#define BAL_STAT_ABORT_MASK     0x04
#define BAL_CTRL2_BAL_GO_MASK   0x02
#define BAL_CTRL2_CONFIG        0x31    // auto_bal = 1, otcb_en = 1, flt_stop = 1

void StackBalancingConfig(DbmsCtx* ctx)
{
    // Setting balancing method to auto balancing and to stop at fault
    uint8_t frame_cb_config[] = {0xB0, 0x03, 0x2F, BAL_CTRL2_CONFIG, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_cb_config, sizeof(frame_cb_config));
}


void StackSetDeviceBalanceTimer(DbmsCtx* ctx, uint8_t device_addr, bool cells_to_balance[N_GROUPS_PER_SIDE])
{
    uint16_t base_reg = 0x0327;  // CB_CELL1_CTRL (starts at cell 1, goes down)
    
    // Write each cell timer individually
    for (size_t i = 0; i < N_GROUPS_PER_SIDE; ++i)
    {
        uint16_t reg_addr = base_reg - i; // registers decrement
        uint8_t timer_val = cells_to_balance[i] ? TIMER_VALUE : 0x00;
        
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
 *   3 step process -
 *   1. set up individual cell timers (write non-zero values to cb_cell_ctrl registers) - need to experiment with timer
 * values
 *   2. configure balancing method
 *   3. set additional controls (write to bal_ctrl2 register to configure auto or manual balancing)
 */
void StackStartBalancing(DbmsCtx* ctx)
{
    for (size_t segment = 0; segment < N_SEGMENTS; ++segment)
    {
        for (size_t side_in_seg = 0; side_in_seg < N_SIDES_PER_SEG; ++side_in_seg)
        {
            size_t side_index = segment * N_SIDES_PER_SEG + side_in_seg;

            for(size_t monitor = 0; monitor < N_MONITORS_PER_SIDE; ++monitor)
            {
                uint8_t device_addr = (side_index * N_MONITORS_PER_SIDE) + monitor;
                
                StackSetDeviceBalanceTimer(ctx, device_addr, ctx->cell_states[side_index].cells_to_balance);
                StackStartDeviceBalancing(ctx, device_addr);
            }
        }
    }
}


bool StackIsDoneBalancing(DbmsCtx* ctx)
{
    uint8_t bal_stat_frame[] = {0xA0, 0x05, 0x2B, 0x00, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, bal_stat_frame, sizeof(bal_stat_frame));

    uint8_t rx_buffer[RX_FRAME_SIZE(1) * N_MONITORS];

    if (HAL_UART_Receive(ctx->hw.uart, rx_buffer, sizeof(rx_buffer), STACK_RECV_TIMEOUT) != HAL_OK)
    {
        // throw error
        return false;
    }

    // RxStackFrame rx_frames[N_MONITORS];
    // FillStackFrames(rx_frames, rx_buffer, sizeof(rx_buffer), N_MONITORS);
    // uint8_t bal_stat_value = 0;
    // for (size_t i = 0; i < N_MONITORS; ++i)
    // {
    //     bal_stat_value = rx_frames[i].data[0];

    //     if ((bal_stat_value & CB_RUN_BIT_MASK) != 0)
    //     {
    //         return false; // Still balancing
    //     }
    // }
    // return true;

    return false;
}
