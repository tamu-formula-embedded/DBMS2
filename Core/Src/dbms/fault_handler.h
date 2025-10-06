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
    CTRL_FAULT_VOLTAGE_OVER = 0, // 1...
    CTRL_FAULT_VOLTAGE_UNDER = 1,
    CTRL_FAULT_TEMP_OVER = 2,
    CTRL_FAULT_TEMP_UNDER = 3,
    CTRL_FAULT_CURRENT_OVER = 4,
    CTRL_FAULT_CURRENT_UNDER = 5,
    CTRL_FAULT_PACK_VOLTAGE_OVER = 6,
    CTRL_FAULT_PACK_VOLTAGE_UNDER = 7,
    CTRL_FAULT_MAX_DELTA_EXCEEDED = 8,
    CTRL_FAULT_STACK_FAULT = 9,
    CTRL_FAULT_CURRENT_PULSE = 10,
    CTRL_FAULT_TYPE_COUNT // Total number of fault types -- should be last
} ControllerFaultType;

typedef enum
{
    BRIDGE_FAULT_CONF_MON_ERR = 0, //could not find
    BRIDGE_FAULT_FACTLDERR,
    BRIDGE_FAULT_FACT_CRC,
    BRIDGE_FAULT_VALIDATE_DET, //could not find
    BRIDGE_FAULT_LFO,
    BRIDGE_FAULT_SHUTDOWN_REC, //reserved
    BRIDGE_FAULT_DRST,
    BRIDGE_FAULT_CTL,
    BRIDGE_FAULT_CTS,
    BRIDGE_FAULT_TSHUT,
    BRIDGE_FAULT_INH, //could not find
    BRIDGE_FAULT_CVDD_UV_DRST, // could not find
    BRIDGE_FAULT_CVDD_OV,
    BRIDGE_FAULT_DVDD_OV,
    BRIDGE_FAULT_AVDDREF_OV, // could not find
    BRIDGE_FAULT_FCOMM_DET, 
    BRIDGE_FAULT_FTONE_DET,
    BRIDGE_FAULT_HB_FAIL,
    BRIDGE_FAULT_HB_FAST,
    BRIDGE_FAULT_UART_FRAME, // could not find
    BRIDGE_FAULT_COMMCRL_DET, // could not find
    BRIDGE_FAULT_STOP_DET,
    BRIDGE_FAULT_AVAO_SW_FAIL, // could not find
    BRIDGE_FAULT_COML_FRAME, //by bit
    BRIDGE_FAULT_COML_PHY, //PHY?
    BRIDGE_FAULT_COMH_FRAME, // by bit
    BRIDGE_FAULT_COMH_PHY, // PHY?
    BRIDGE_FAULT_TYPE_COUNT // Total number of fault types -- should be last
} BridgeFault;

typedef enum{
    BRIDGE_FAULT_COMM,
    BRIDGE_FAULT_REG,
    BRIDGE_FAULT_SYS,
    BRIDGE_FAULT_PWR,
} BridgeFaultType;

typedef enum{
    MONITOR_FAULT_PWR,
    MONITOR_FAULT_SYS,
    MONITOR_FAULT_OVUV,
    MONITOR_FAULT_OTUT,
    MONITOR_FAULT_COMM,
    MONITOR_FAULT_OTP,
    MONITOR_FAULT_COMP_ADC,
    MONITOR_FAULT_PROT
} MonitorFaultType;

void ControllerSetFault(DbmsCtx* ctx, ControllerFaultType fault);
void ControllerClearFault(DbmsCtx* ctx, ControllerFaultType fault);
bool ControllerHasFault(DbmsCtx* ctx, ControllerFaultType fault);

void CheckVoltageFaults(DbmsCtx* ctx);
void CheckTemperatureFaults(DbmsCtx* ctx);
void CheckCurrentFaults(DbmsCtx* ctx);

// void BridgeSetFaultSummary(DbmsCtx* ctx, uint8_t fault_summary_reg);
// void BridgeSetFault(DbmsCtx* ctx, BridgeFault fault);
// void BridgeClearFault(DbmsCtx* ctx, BridgeFault fault);
// bool BridgeHasFault(DbmsCtx* ctx, BridgeFault fault);

// void StackSetFaultSummary(DbmsCtx* ctx, uint8_t addr, uint8_t fault_summary_reg);
// void StackSetFault(DbmsCtx* ctx, uint8_t addr, MonitorFaultType fault); // todo: make it so each category has its own fault storage
// void StackClearFault(DbmsCtx* ctx, uint8_t addr, MonitorFaultType fault); //not implemented
// bool StackHasFault(DbmsCtx* ctx, uint8_t addr, MonitorFaultType fault); //not implemented

bool HasAnyFaults(DbmsCtx* ctx);
void ClearAllFaults(DbmsCtx* ctx);

// Comm to the GPIO
void ThrowHardFault(DbmsCtx* ctx);

int LoadFaultState(DbmsCtx* ctx);
int SaveFaultState(DbmsCtx* ctx);

#endif
