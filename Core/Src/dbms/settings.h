//  
//  Copyright (c) Texas A&M University.
//  
#ifndef _H_SETTINGS_H
#define _H_SETTINGS_H

#include "common.h"
#include "context.h"
#include "storage.h"

// these objects are fwd declared

enum _UserSettingIndex {
    UNUSED1 = 0,

    QUIET_MS_BEFORE_SHUTDOWN,
    MAX_GROUP_VOLTAGE,
    MIN_GROUP_VOLTAGE,

    __NUM_USER_DEFINED_SETTINGS     // ALWAYS LAST
}; 

/**
 * This struct is legacy -- the settings used to be items in
 * the struct, but user defined settings were changed to 
 * use an array
 */
struct _DbmsSettings {

    uint32_t user_defined[__NUM_USER_DEFINED_SETTINGS];

};

uint32_t GetSetting(DbmsCtx* ctx, UserSettingIndex index);
void SetSetting(DbmsCtx* ctx, UserSettingIndex index, uint32_t new_val);
bool IsValidSettingIndex(DbmsCtx* ctx, uint32_t index);

int LoadSettings(DbmsCtx* ctx);
int SaveSettings(DbmsCtx* ctx);

void LoadFallbackSettings(DbmsCtx* ctx);

#endif