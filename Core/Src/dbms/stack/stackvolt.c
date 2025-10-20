#include "stack.h"

/**
 * Enable the voltage tap ADCs
 */
void StackSetupVoltReadings(DbmsCtx* ctx)
{
    static uint8_t FRAME_SET_ADC_CONT_RUN[] = {0xD0, 0x03, 0x0D, 0x06, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, FRAME_SET_ADC_CONT_RUN, sizeof(FRAME_SET_ADC_CONT_RUN));
}

/**
 * Request and populate voltage readings for a monitor by side-addr
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
    // TODO: redo the RX path for stack
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