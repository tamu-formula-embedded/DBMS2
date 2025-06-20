#include "dbms.h"

void DbmsInit(DbmsCtx* ctx, HwCtx* hw)
{
    ConfigCan(hw);

    static uint8_t msg_init[] = {'I', 'n', 'i', 't', 0, 0, 0, 0};

    CanTransmit(hw, 0x5F1, msg_init);
}

void DbmsIter(DbmsCtx* ctx, HwCtx* hw)
{
    static uint8_t msg_iter[] = {'I', 't', 'e', 'r', 0, 0, 0, 0};
    CanTransmit(hw, 0x5F2, msg_iter);
}

void DbmsErr(DbmsCtx* ctx, HwCtx* hw)
{

}