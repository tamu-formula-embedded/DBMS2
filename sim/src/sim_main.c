#include "dbms/dbms.h"
#include <stdio.h>
#include <stdlib.h>

// Simulation entry point
int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <first_arg> <second_arg>\n", argv[0]);
        return 1;
    }

    __SimEnter(argv[1], argv[2]);

    DbmsCtx ctx = {0};
    ADC_HandleTypeDef adc = {0};
    TIM_HandleTypeDef timer = {0};
    UART_HandleTypeDef uart = {0};
    CAN_HandleTypeDef can = {0};
    HwCtx hw = {0};
    hw.adc = &adc;
    hw.timer = &timer;
    hw.uart = &uart;
    hw.can = &can;

    DbmsInit(&ctx, &hw);

    while (1)
    {
        DbmsIter(&ctx, &hw);
    }

    __SimExit();
}
