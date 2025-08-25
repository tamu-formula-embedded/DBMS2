//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _FAULT_HANDLER_H_
#define _FAULT_HANDLER_H_

#include "common.h"
#include "context.h"

/*
 *  Fault types, ordinal determines position in bitmask
 */
typedef enum {
    FAULT_VOLTAGE_OVER = 0,      
    FAULT_VOLTAGE_UNDER = 1,     
    FAULT_TEMP_OVER = 2,         
    FAULT_TEMP_UNDER = 3,        
    FAULT_CURRENT_OVER = 4,      
    FAULT_CURRENT_UNDER = 5,     
    FAULT_PACK_VOLTAGE_OVER = 6, 
    FAULT_PACK_VOLTAGE_UNDER = 7,
    FAULT_TYPE_COUNT             // Total number of fault types -- should be last
} FaultType;

void SetFault(DbmsCtx* ctx, FaultType fault);
void ClearFault(DbmsCtx* ctx, FaultType fault);
bool HasFault(DbmsCtx* ctx, FaultType fault);
#define HAS_ANY_FAULTS(CTX) (((DbmsCtx*)CTX)->fault_mask != 0)
#define CLEAR_ALL_FAULTS(CTX) (((DbmsCtx*)CTX)->fault_mask = 0)

// Fault checking functions
void CheckVoltageFaults(DbmsCtx* ctx);
void CheckTemperatureFaults(DbmsCtx* ctx);
void CheckCurrentFaults(DbmsCtx* ctx);

#endif 
