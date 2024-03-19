#pragma once

#include "resourcesTypes.h"

typedef struct resourceManagerSettings {
    u32 maxManagers;
    char* rootAssetPath;
} resourceManagerSettings;

typedef struct resourceManager {
    u32 id;
    ResourceType resourceType;
    const char* customTypeName;
    b8 (*load)(struct resourceManager* self, const char* name, resource* outResource);
    void (*unload)(struct resourceManager* self, resource* resource);
} resourceManager;

b8 resourceManagerInit(u64* memoryRequirement, void* state, resourceManagerSettings settings);
void resourceManagerShutdown(void* state);

b8 resourceManagerLoadManager(resourceManager manager);

b8 resourceLoad(const char* name, ResourceType type, resource* outResource);
b8 resourceUnload(resource* resource);

char* resourceManagerRootAssetPath();
void resourceManagerChangeRootAssetPath(char* newRootAssetPath);
