#include "settings.h"

uint32_t GetSetting(DbmsCtx* ctx, UserSettingIndex index)
{
    return ctx->settings->user_defined[index];
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
    // optional implementation
}


