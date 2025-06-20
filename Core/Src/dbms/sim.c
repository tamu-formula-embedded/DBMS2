#ifdef SIM

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __clang__
    typedef int int32_t;
    typedef unsigned int uint32_t;
#endif

#include "sim.h"

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *sFilterConfig)
{
    return HAL_OK;  // explicit do nothing
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
    return HAL_OK;  // explicit do nothing
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan)
{
    return 0;       // explicit do nothing
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[], uint32_t *pTxMailbox)
                                       
{
    printf("CAN | %03x | %02x | %02x | %02x | %02x | %02x | %02x | %02x | %02x\n",
        pHeader->StdId, aData[0], aData[1], aData[2], aData[3], aData[4], aData[5], aData[6], aData[7]);
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    return HAL_OK;  // explicit do nothing
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout)
{
    return HAL_OK;  // explicit do nothing
}

static int i = 0;

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    return (i++) % 1000;
}

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc)
{
    return HAL_OK;  // explicit do nothing
}


#endif 