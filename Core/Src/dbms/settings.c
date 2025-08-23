#include "settings.h"

uint32_t GetSetting(DbmsCtx* ctx, UserSettingIndex index)
{
    // CanLog(ctx, "GS %d %c\n", index, ctx->settings ? 'Y' : 'N');
    if (ctx->settings != NULL)
        return ctx->settings->user_defined[index];
    else {
        CanLog(ctx, "SETING NULL %d\n", index);
        return 1000;
    }
}

void SetSetting(DbmsCtx* ctx, UserSettingIndex index, uint32_t new_val)
{
    ctx->settings->user_defined[index] = new_val;
}

bool IsValidSettingIndex(DbmsCtx* ctx, uint32_t index)
{
    return index < sizeof(ctx->settings->user_defined);
}

int LoadSettings(DbmsCtx* ctx)
{
    return LoadStoredObject(ctx, EEPROM_SETTINGS_ADDR, ctx->settings, sizeof(DbmsSettings));
}

int SaveSettings(DbmsCtx* ctx)
{
    return SaveStoredObject(ctx, EEPROM_SETTINGS_ADDR, ctx->settings, sizeof(DbmsSettings));
}

void LoadFallbackSettings(DbmsCtx* ctx)
{
    ctx->settings->user_defined[QUIET_MS_BEFORE_SHUTDOWN] = 2000;   // 2s

    ctx->settings->user_defined[MAX_GROUP_VOLTAGE] = 5000;          // 1v
    ctx->settings->user_defined[MIN_GROUP_VOLTAGE] = 1000;          // 5v
    ctx->settings->user_defined[MAX_PACK_VOLTAGE] = 600000;         // 600v
    ctx->settings->user_defined[MIN_PACK_VOLTAGE] = 30000;          // 30v

    ctx->settings->user_defined[MAX_THERMISTOR_TEMP] = 1000;        // todo: ?
    ctx->settings->user_defined[MAX_CURRENT] = 100000;              // todo: ?
}


