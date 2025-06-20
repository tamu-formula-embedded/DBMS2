
#include "dbms/dbms.h"

// Simulation entry point
int main(int argc, char** argv)
{
    DbmsCtx ctx = {0};
    HwCtx hw = {0};

    DbmsInit(&ctx, &hw);

    while (1)
    {
        DbmsIter(&ctx, &hw);
    }
}