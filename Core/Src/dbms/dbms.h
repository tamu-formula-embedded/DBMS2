#ifndef _DBMS_H_
#define _DBMS_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct _DbmsSettings {

    // The maximum voltage of the pack or else a fault is thrown
    uint16_t max_allowed_pack_voltage;

} DbmsSettings;



typedef struct _DbmsCtx {

    DbmsSettings settings;

} DbmsCtx;


// Called before the main loop
void DbmsInit(DbmsCtx* ctx);

// Called from the main loop
void DbmsIter(DbmsCtx* ctx);

// Called to handle an err state
void DbmsErr(DbmsCtx* ctx);

#endif