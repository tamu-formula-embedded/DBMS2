#ifndef DBMS_DATA_H
#define DBMS_DATA_H

#include "context.h"

void DataInit(DbmsCtx* ctx);
float VoltageToTemperature(DbmsCtx* ctx, float voltage);

#endif
