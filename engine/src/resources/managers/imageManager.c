#include "imageManager.h"
#include "core/logger.h"
#include "core/fstring.h"
#include "core/fmemory.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "dependencies/stb_image.h"

b8 imageManagerLoad(resourceManager* self, const char* name, resource* outResource){
    char* formatStr = "%s/%s";
    const i32 reqChannelCnt = 4;
    stbi_set_flip_vertically_on_load(true);
    char fullFilePath[512];

    strFmt(fullFilePath, formatStr, resourceManagerRootAssetPath(), name);

    i32 width;
    i32 height;
    i32 channelCnt;

    u8* data = stbi_load(
        fullFilePath,
        &width,
        &height,
        &channelCnt,
        reqChannelCnt);

    const char* failReason = stbi_failure_reason();

    if (data == NULL && failReason){
        FERROR("Image Manager failed to get image %s", name);
        stbi__err(0,0);
        if (data){
            stbi_image_free(data);
        }
        return false;
    }

    outResource->fullPath = strDup(fullFilePath);

    imageRS* irs = fallocate(sizeof(imageRS), MEMORY_TAG_TEXTURE);
    irs->width = width;
    irs->height = height;
    //TODO: Make it so it can use only 3 channels.
    irs->channelCnt = reqChannelCnt;
    //irs->channelCnt = channelCnt;
    irs->pixels = data;

    outResource->data = irs;
    outResource->dataSize = sizeof(imageRS);
    outResource->name = name;

    return true;
}

void imageManagerUnload(resourceManager* self, resource* resource){
    if (!self || !resource){
        FWARN("Can't unload image without self or resource structs.");
        return;
    }

    u32 len = strLen(resource->fullPath);
    if (len){
        ffree(resource->fullPath, sizeof(char) * len + 1, MEMORY_TAG_STRING);
    }

    if (resource->data){
        ffree(resource->data, resource->dataSize, MEMORY_TAG_TEXTURE);
        resource->dataSize = 0;
        resource->data = 0;
        resource->managerID = INVALID_ID;
    }
}

resourceManager imageManagerCreate(){
    resourceManager m;
    m.resourceType = RESOURCE_TYPE_IMAGE;
    m.id = RESOURCE_TYPE_IMAGE;
    m.load = imageManagerLoad;
    m.unload = imageManagerUnload;
    return m;
}
