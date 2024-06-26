#pragma once

#include "defines.h"
#include "resources/resourcesTypes.h"
#include "math/matrixMath.h"

#define BUILTIN_MATERIAL_PBR_NAME "Builtin.Shader.PBR"
#define BUILTIN_MATERIAL_UI_NAME "Builtin.Shader.UI"
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
material* materialSystemMaterialGetByConfig(materialFileConfig config);
void materialSystemMaterialRelease(const char* name);
b8 materialSystemCreateDefault();
material* materialSystemGetDefault();
b8 materialSystemUpdateGlobal(u32 shaderID, const mat4* proj, const mat4* view);
b8 materialSystemUpdateInstance(material* m);
b8 materialSystemUpdateLocal(material* m, const mat4* model);
