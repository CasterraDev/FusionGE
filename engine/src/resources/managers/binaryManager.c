#include "binaryManager.h"

#include "core/logger.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "resources/resourceManager.h"
#include "resources/resourcesTypes.h"

#include "platform/filesystem.h"

b8 binaryManagerLoad(resourceManager* self, const char* name, resource* outRes){
    if (!self || !name || !outRes){
        return false;
    }

    char* fmtStr = "%s/%s%s";
    char path[512];
    strFmt(path, fmtStr, resourceManagerRootAssetPath(), name, "");

    fileHandle fh;
    if (!fsOpen(path, FILE_MODE_READ, true, &fh)){
        FERROR("Binary Manager unable to load binary file: %s", path);
        return false;
    }

    //TODO: Could be memory leaks
    outRes->fullPath = strDup(path);

    u64 fileSize = 0;
    if (!fsSize(&fh, &fileSize)){
        FERROR("Failed to get binary file size: %s", path);
        return false;
    }

    u8* resData = fallocate(sizeof(u8*) * fileSize, MEMORY_TAG_ARRAY);
    u64 readSize = 0;
    if (!fsReadFileBytes(&fh, resData, &readSize)){
        FERROR("Failed to read binary file: %s", path);
        fsClose(&fh);
        return false;
    }

    fsClose(&fh);

    outRes->data = resData;
    outRes->dataSize = readSize;
    outRes->name = name;

    return true;
}

void binaryManagerUnload(resourceManager* self, resource* res){
    if (!self || !res){
        FWARN("BinaryManagerUnload - Manager or Resource is null.");
        return;
    }

    u32 pathLen = strLen(res->fullPath);
    if (pathLen){
        ffree(res->fullPath, sizeof(char) * pathLen + 1, MEMORY_TAG_STRING);
    }

    if (res->data){
        ffree(res->data, res->dataSize, MEMORY_TAG_ARRAY);
        res->data = 0;
        res->dataSize = 0;
        res->managerID = INVALID_ID;
    }
}

resourceManager binaryManagerCreate(){
    resourceManager m;
    m.resourceType = RESOURCE_TYPE_BINARY;
    m.id = RESOURCE_TYPE_BINARY;
    m.load = binaryManagerLoad;
    m.unload = binaryManagerUnload;
    return m;
}
