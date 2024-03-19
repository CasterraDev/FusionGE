#pragma once

#include "defines.h"
#include "resources/resourcesTypes.h"
#include "math/matrixMath.h"

typedef struct materialSystemSettings{
    u64 maxMaterialCnt;
} materialSystemSettings;

typedef struct materialFileConfig{
    char name[FILENAME_MAX_LENGTH];
    u32 shaderID;
    b8 autoDelete;
    //TODO: TEMP
    vector4 diffuseColor;
    char DiffuseMapName[128];
} materialFileConfig;

void materialSystemInit(u64* memoryRequirement, void* state, materialSystemSettings settings);
void materialSystemShutdown(void* state);
material* materialSystemMaterialGet(const char* name);
material* materialSystemMaterialGetFromConfig(const char* name);
void materialSystemMaterialRelease(const char* name);
b8 materialSystemCreateDefault();
material* materialSystemGetDefault();
