#include "stack.h"

// TODO: rewrite this module when we revisit stack faults

// BEGIN LEGACY
typedef struct
{ // ordering packed
    uint8_t* data;
    size_t size;
    uint16_t reg_addr;
    uint16_t crc;
    uint8_t init_field;
    uint8_t dev_addr;
} RxStackFrame;

uint16_t CalcStackFrameCrc(RxStackFrame* rx_frame)
{
    // TODO: this data-4 is a STUPID decision but will fix once reg_addr endian is validated
    return CalcCrc16(rx_frame->data - 4, 4 + rx_frame->size);
}

void FillStackFrame(RxStackFrame* rx_frame, uint8_t* buffer, size_t size)
{
    rx_frame->init_field = buffer[0];
    rx_frame->dev_addr = buffer[1];
    rx_frame->reg_addr = (buffer[2] << 8) + buffer[3];
    rx_frame->data = buffer + 4;
    rx_frame->size = size;
    rx_frame->crc = (buffer[4 + size + 1] << 8) + buffer[4 + size];
}

void FillStackFrames(RxStackFrame* rx_frames, uint8_t* buffer, size_t size, size_t n_frames)
{
    for (size_t i = 0; i < n_frames; i++)
    {
        FillStackFrame(rx_frames + i, buffer + (i * RX_FRAME_SIZE(size)), size);
    }
}
// END LEGACY 

int SetFaultMasks(DbmsCtx* ctx)
{
    int status = 0;

    uint8_t FRAME_MASK_CRC[] = {0xD0, 0x00, 0x17, 0x40, 0x00, 0x00};

    if ((status = SendStackFrameSetCrc(ctx, FRAME_MASK_CRC, sizeof(FRAME_MASK_CRC))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }
    
    return status;
}


#define FAULT_SUMM_BITMASK 0b11101100   // DO NOT CHANGE WITHOUT TALKING 
                                        // WITH THE TEAM FIRST PLSSS!!

// TODO: clean this one up when we look into this stuff more

int PollFaultSummary(DbmsCtx* ctx)
{
    static uint8_t rx_fault_readings[1024];
    int status = 0;
    size_t data_size = 1 * sizeof(uint8_t);
    // TODO add functions to go through all the other fault registers if fault_summary says something is wrong
    uint8_t FRAME_POLL_CTRL_FAULT_SUMMARY[] = {0xC0, 0x05, 0x2D, 0x00, 0x00, 0x00};

    if ((status = SendStackFrameSetCrc(ctx, FRAME_POLL_CTRL_FAULT_SUMMARY, sizeof(FRAME_POLL_CTRL_FAULT_SUMMARY))) != 0)
    {
        CAN_REPORT_FAULT(ctx, status);
    }

    size_t expected_rx_size = RX_FRAME_SIZE(data_size) * N_STACKDEVS; // <- num frames

    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_fault_readings, expected_rx_size, 50)) != 0)
    {
        // CAN_REPORT_FAULT(ctx, status);
        // ^ throwing all the time in simulator
    }

    RxStackFrame rx_frames[N_STACKDEVS];
    FillStackFrames(rx_frames, rx_fault_readings, data_size, N_MONITORS);
    uint16_t addr, kcrc;

    for (size_t i = 0; i < N_STACKDEVS; i++)
    {
        ctx->stats.n_rx_stack_frames++;

        // Excluding this since bridge device can also have faults that 
        if (rx_frames[i].dev_addr == 0) continue; // this is myself
        addr = rx_frames[i].dev_addr - 1;         // ignore the controller from a broadcast
        if (addr >= N_MONITORS) continue;         // throw some error here

        if (rx_frames[i].crc == (kcrc = CalcStackFrameCrc(&(rx_frames[i]))))
        {
            ctx->faults.monitor_fault_summary[i] = rx_frames[i].data[0] & FAULT_SUMM_BITMASK;
            if (ctx->faults.monitor_fault_summary[i])
            {
                ControllerSetFault(ctx, CTRL_FAULT_STACK_FAULT);
                CanLog(ctx, "F %d %x\n", addr, rx_frames[i].data[0]);
            }
        }
        else
        {
            ctx->stats.n_rx_stack_bad_crcs++;
        }
    }
    return status;
}

