#include "dbms/dbms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// If you don't have sim.h to include, declare the sim API here:
#ifdef SIM
// context enter/exit
void __SimEnter(char* ipc_path_can, char* ipc_path_uart);
void __SimExit(void);

// polling & pop (no Dbms calls inside sim.c)
typedef struct {
    uint32_t id_be;  // 4B big-endian ID already parsed to host order in sim.c
    uint8_t  data[8];
} SimCanFrame;

void __SimCanPoll(void);
int  __SimCanPop(SimCanFrame* out);
#endif

// Simulation entry point
int main(int argc, char** argv)
{
#ifdef SIM
    // Fixed TCP sockets inside __SimEnter (CAN=4002, UART=4001)
    __SimEnter(NULL, NULL);
#endif

    DbmsCtx ctx = {0};
    ADC_HandleTypeDef adc = {0};
    TIM_HandleTypeDef timer = {0};
    UART_HandleTypeDef uart = {0};
    CAN_HandleTypeDef can = {0};

    HwCtx hw = {0};
    hw.adc   = &adc;
    hw.timer = &timer;
    hw.uart  = &uart;
    hw.can   = &can;

    DbmsAlloc(&ctx);
    DbmsInit(&ctx);

    for (;;)
    {
#ifdef SIM
        // 1) Pull any incoming CAN bytes into frame queue
        __SimCanPoll();

        // 2) Dispatch all queued frames to firmware ISR endpoint
        SimCanFrame f;
        while (__SimCanPop(&f))
        {
            CAN_RxHeaderTypeDef rxh;
            memset(&rxh, 0, sizeof(rxh));

            // Your wire format is 4B id (we used StdId when sending).
            // Keep only 11-bit standard ID for StdId:
            uint32_t id_wire = f.id_be;
            rxh.StdId = (uint16_t)(id_wire & 0x7FFu);
            rxh.DLC   = 8;
            rxh.IDE   = 0; // standard

            DbmsCanRx(&ctx, (CanRxChannel)0, rxh, f.data);
        }
#endif

        // 3) Main iteration
        DbmsIter(&ctx);

        // (Optional) small sleep/yield if your loop is too hot
        // usleep(1000);
    }

#ifdef SIM
    __SimExit();
#endif
    return 0;
}
