
#ifndef _FAULTS_H_ 
#define _FAULTS_H_

#include "../utils/common.h"
#include "../context.h"
#include "../settings.h"
#include "../vinterface.h"

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
} CtrlFault;

void CtrlSetFault(DbmsCtx* ctx, CtrlFault fault);
void CtrlClearFault(DbmsCtx* ctx, CtrlFault fault);
bool CtrlHasFault(DbmsCtx* ctx, CtrlFault fault);
bool CtrlHasAnyFaults(DbmsCtx* ctx);
void CtrlClearAllFaults(DbmsCtx* ctx);

void CheckVoltageFaults(DbmsCtx* ctx);
void CheckTemperatureFaults(DbmsCtx* ctx);
void CheckCurrentFaults(DbmsCtx* ctx);

void SetFaultLine(DbmsCtx* ctx, bool faulted);
void ThrowHardFault(DbmsCtx* ctx);
int LoadFaultState(DbmsCtx* ctx);
int SaveFaultState(DbmsCtx* ctx);



#endif