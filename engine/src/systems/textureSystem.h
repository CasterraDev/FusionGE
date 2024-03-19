#pragma once

#include "defines.h"
#include "resources/resourcesTypes.h"

typedef struct textureSystemSettings {
    u64 maxTextureCnt;
} textureSystemSettings;

typedef struct textureInfo {
    u64 refCnt;
    u32 idx;
    texture tex;
    b8 autoDelete;
} textureInfo;


void textureSystemInit(u64* memoryRequirement, void* state, textureSystemSettings settings);
void textureSystemShutdown(void* state);
texture* textureSystemTextureGetCreate(const char* name, b8 autoDelete);
texture* textureSystemTextureGet(const char* name, b8 autoDelete);
void textureSystemTextureRelease(const char* name);
b8 textureSystemCreateDefault();

texture* textureSystemGetDefault();
