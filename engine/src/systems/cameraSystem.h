#pragma once
#include "defines.h"
#include "renderer/camera.h"

#define HashtableDefaultValue INVALID_ID

typedef struct camLookup{
    u32 id;
    char name[32];
    camera cam;
} camLookup;

void cameraSystemInit(u64* memoryRequirement, void* state, u32 maxCameras);
void cameraSystemShutdown(void* state);

void cameraSystemAdd(const char* name);
void cameraSystemDelete(const char* name);

camera* getCamera(const char* name);
