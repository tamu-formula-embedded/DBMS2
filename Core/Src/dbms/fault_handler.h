//
//  Copyright (c) Texas A&M University.
//
#ifndef _CTRL_FAULT_HANDLER_H_
#define _CTRL_FAULT_HANDLER_H_

#include "common.h"
#include "context.h"

/*
 *  Fault types, ordinal determines position in bitmask
 */
typedef enum
{
    CTRL_FAULT_VOLTAGE_OVER = 0,
    CTRL_FAULT_VOLTAGE_UNDER = 1,
    CTRL_FAULT_TEMP_OVER = 2,
    CTRL_FAULT_TEMP_UNDER = 3,
    CTRL_FAULT_CURRENT_OVER = 4,
    CTRL_FAULT_CURRENT_UNDER = 5,
    CTRL_FAULT_PACK_VOLTAGE_OVER = 6,
    CTRL_FAULT_PACK_VOLTAGE_UNDER = 7,
    CTRL_FAULT_TYPE_COUNT // Total number of fault types -- should be last
} ControllerFaultType;

typedef enum
{
    STACK_FAULT_CONF_MON_ERR = 0, //could not find
    STACK_FAULT_PWR,
    STACK_FAULT_SYS,
    STACK_FAULT_OVUV,
    STACK_FAULT_OTUT,
    STACK_FAULT_COMM,
    STACK_FAULT_OTP,
    STACK_FAULT_COMP_ADC,
    STACK_FAULT_PROT,
    STACK_FAULT_FACTLDERR,
    STACK_FAULT_FACT_CRC,
    STACK_FAULT_VALIDATE_DET, //could not find
    STACK_FAULT_LFO,
    STACK_FAULT_SHUTDOWN_REC, //reserved
    STACK_FAULT_DRST,
    STACK_FAULT_CTL,
    STACK_FAULT_CTS,
    STACK_FAULT_TSHUT,
    STACK_FAULT_INH, //could not find
    STACK_FAULT_CVDD_UV_DRST, // could not find
    STACK_FAULT_CVDD_OV,
    STACK_FAULT_DVDD_OV,
    STACK_FAULT_AVDDREF_OV, // could not find
    STACK_FAULT_FCOMM_DET, 
    STACK_FAULT_FTONE_DET,
    STACK_FAULT_HB_FAIL,
    STACK_FAULT_HB_FAST,
    STACK_FAULT_UART_FRAME, // could not find
    STACK_FAULT_COMMCRL_DET, // could not find
    STACK_FAULT_STOP_DET,
    STACK_FAULT_AVAO_SW_FAIL, // could not find
    STACK_FAULT_COML_FRAME, //by bit
    STACK_FAULT_COML_PHY, //PHY?
    STACK_FAULT_COMH_FRAME, // by bit
    STACK_FAULT_COMH_PHY, // PHY?
    STACK_FAULT_TYPE_COUNT // Total number of fault types -- should be last
} StackFaultType;

void ControllerSetFault(DbmsCtx* ctx, ControllerFaultType fault);
void ControllerClearFault(DbmsCtx* ctx, ControllerFaultType fault);
bool ControllerHasFault(DbmsCtx* ctx, ControllerFaultType fault);

void CheckVoltageFaults(DbmsCtx* ctx);
void CheckTemperatureFaults(DbmsCtx* ctx);
void CheckCurrentFaults(DbmsCtx* ctx);

void StackSetFault(DbmsCtx* ctx, uint8_t addr, StackFaultType fault);
void StackClearFault(DbmsCtx* ctx, uint8_t addr, StackFaultType fault);
bool StackHasFault(DbmsCtx* ctx, uint8_t addr, StackFaultType fault);

bool HasAnyFaults(DbmsCtx* ctx);
void ClearAllFaults(DbmsCtx* ctx);

// Comm to the GPIO
void ThrowHardFault(DbmsCtx* ctx);

int LoadFaultState(DbmsCtx* ctx);
int SaveFaultState(DbmsCtx* ctx);

#endif
