#include "stack.h"

/**
 * Configure GPIOs for temp readings
 */
void StackSetupGpio(DbmsCtx* ctx)
{
    // TODO: implement
    // Configures all GPIOs to be ADC and OTUT input
    // Note: Can be made more effecient by using NVM
    // Note 2: Also configures GPIO8 even though we dont need to, hence just setting GPIO8 to output low
    uint8_t frame_gpio_configs[] = {0xB3, 0x00, 0x0E, 0x09, 0x09, 0x09, 0x21, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_gpio_configs, sizeof(frame_gpio_configs));

    // Setting up TSREF to active
    uint8_t frame_tsref_setup[] = {0xB0, 0x03, 0x0A, 0x01, 0x00, 0x00};
    SendStackFrameSetCrc(ctx, frame_tsref_setup, sizeof(frame_tsref_setup));
    DelayUs(ctx, 10);
}

/**
 * Read temperatures from a monitor by side-addr and if/if not it is the sidekick monitor
 * 
 * TODO: more explaination 
 */
void StackUpdateTempReadingSingle(DbmsCtx* ctx, uint16_t addr, bool sidekick)
{
    static uint8_t rx_buffer_t[1024];
    int status = 0;

    uint8_t in_addr = addr * 2 + 1 + (sidekick ? 1 : 0);
    uint8_t offset = sidekick ? N_TEMPS_PER_MONITOR : 0;
    uint8_t frame[] = {0x80, in_addr, 0x05, 0x8E, 0x0D, 0x00, 0x00}; // Reading 14 bytes

    size_t data_size = N_TEMPS_PER_MONITOR * sizeof(int16_t);
    size_t expected_rx_size = RX_FRAME_SIZE(data_size);

    if ((status = SendStackFrameSetCrc(ctx, frame, sizeof(frame))) != 0) { }
    if ((status = HAL_UART_Receive(ctx->hw.uart, rx_buffer_t, expected_rx_size+2, STACK_RECV_TIMEOUT)) != 0) { } 
    ctx->stats.n_rx_stack_frames++;

    uint8_t* data = rx_buffer_t;
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

    for (size_t j = 0; j < N_TEMPS_PER_MONITOR; j++)
    {
        uint16_t raw = (data[j * sizeof(int16_t)] << 8) + (data[j * sizeof(int16_t) + 1]);
        ctx->cell_states[addr].temps[j + offset] = ThermVoltToTemp(ctx, MAX(0, (raw * STACK_T_UV_PER_BIT) / 1000000.0));
    }
}


// (legacy comment)
//
// votlages ->  if (addr % 2 == 0)
//                  monitor id = addr / 2
//                  write(0...N_GROUPS_PER_SIDE)

// temps ->     monitor id = addr / 2
//              if (addr % 2 == 0)
//                  write(0...N_TEMPS_PER_SIDE)
//              else
//                  write(N_TEMPS_PER_SIDE+1...2*N_TEMPS_PER_SIDE)


/**
 * Replace missing thermistor readings with the average of the valid cells
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

