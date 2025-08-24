#ifndef _FAULT_HANDLER_H_
#define _FAULT_HANDLER_H_

#include "common.h"
#include "context.h"

// Fault types as bit positions for bitmask
typedef enum {
    FAULT_VOLTAGE_OVER = 0,      // Cell voltage too high
    FAULT_VOLTAGE_UNDER = 1,     // Cell voltage too low
    FAULT_TEMP_OVER = 2,         // Temperature too high
    FAULT_TEMP_UNDER = 3,        // Temperature too low
    FAULT_CURRENT_OVER = 4,      // Current too high (discharge)
    FAULT_CURRENT_UNDER = 5,     // Current too low (charge)
    FAULT_PACK_VOLTAGE_OVER = 6, // Pack voltage too high
    FAULT_PACK_VOLTAGE_UNDER = 7,// Pack voltage too low
    // ...
    FAULT_TYPE_COUNT             // Total number of fault types
} FaultType;

// Fault checking functions
void CheckVoltageFaults(DbmsCtx* ctx);
void CheckTemperatureFaults(DbmsCtx* ctx);
void CheckCurrentFaults(DbmsCtx* ctx);
void CheckAllFaults(DbmsCtx* ctx);

// Fault utility functions
void SetFault(DbmsCtx* ctx, FaultType fault);
void ClearFault(DbmsCtx* ctx, FaultType fault);
bool HasFault(DbmsCtx* ctx, FaultType fault);
bool HasAnyFaults(DbmsCtx* ctx);
uint32_t GetFaultMask(DbmsCtx* ctx);
void ClearAllFaults(DbmsCtx* ctx);

#endif // _FAULT_HANDLER_H_
