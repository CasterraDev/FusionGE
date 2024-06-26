#include "helpers/hashtable.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "cameraSystem.h"

typedef struct cameraSystemState {
    u32 maxCameras;
    hashtable cameraIDs;
    void* hashtableMemory;
    camLookup* cameras;
    u32 camCnt;

    camera mainCam;
} cameraSystemState;

static cameraSystemState* systemPtr;

void cameraSystemInit(u64* memoryRequirement, void* state, u32 maxCameras){
    // Block of memory will contain state structure, then block for array, then block for hashtable.
    *memoryRequirement = sizeof(cameraSystemState) + (sizeof(camLookup) * maxCameras) + (sizeof(entry) * maxCameras);
    if (state == 0){
        return;
    }
    systemPtr = state;

    systemPtr->maxCameras = maxCameras;
    systemPtr->camCnt = 0;
    systemPtr->cameras = state + sizeof(cameraSystemState);
    systemPtr->hashtableMemory = state + sizeof(cameraSystemState) + sizeof(camLookup) * maxCameras;

    hashtableCreate(sizeof(u64), maxCameras, systemPtr->hashtableMemory, false, 0, &systemPtr->cameraIDs);

    systemPtr->mainCam = cameraCreate();

    u32 count = systemPtr->maxCameras;
    for (u32 i = 0; i < count; i++) {
        systemPtr->cameras[i].id = INVALID_ID;
    }
}

void cameraSystemShutdown(void* state){
    systemPtr = 0;
}

void cameraSystemAdd(const char* name){
    if (strEqual(name, "main")){
        FERROR("Cannot add camera with reserved name: main");
        return;
    }
    u64 id = INVALID_ID;
    b8 alreadyCreated = hashtableGetID(&systemPtr->cameraIDs, name, &id);
    if (!alreadyCreated){
        camLookup l;
        l.cam = cameraCreate();
        strNCpy(l.name, name, 32);
        l.id = id;
        systemPtr->cameras[id] = l;
        hashtableSet(&systemPtr->cameraIDs, name, &id);
    }else{
        FWARN("Camera with name: %s has already been added.", name);
        return;
    }
}

void cameraSystemDelete(const char* name){
    if (strEqual(name, "main")){
        FERROR("Cannot delete camera with reserved name: main");
        return;
    }
    hashtableClear(&systemPtr->cameraIDs, name);
}

camera* getCamera(const char* name){
    if (strEqual(name, "main")){
        return &systemPtr->mainCam;
    }
    u64 id;
    b8 alreadyCreated = hashtableGetID(&systemPtr->cameraIDs, name, &id);
    if (alreadyCreated){
        return &systemPtr->cameras[id].cam;
    }
    FERROR("Failed to get camera id with name: %s", name);
    return 0;
}
