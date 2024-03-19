#include "textureSystem.h"
#include "helpers/hashtable.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "core/fmemory.h"
#include "renderer/rendererFront.h"
#include "resources/resourcesTypes.h"
#include "resources/resourceManager.h"

typedef struct textureSystemState
{
    hashtable textureIDs;
    void* hashtableMemory;
    texture* textures;
    textureSystemSettings settings;

    texture defaultTexture;
} textureSystemState;

b8 isNameDefault(const char* name){
    if (strEqual(name, "Fusion-Default-Texture")){
        return true;
    }
    return false;
}

b8 loadTexture(const char* texture_name, texture* t);

textureSystemState* systemPtr;

void textureSystemInit(u64* memoryRequirement, void* state, textureSystemSettings settings){
    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 texturesSize = sizeof(texture) * settings.maxTextureCnt;
    u64 hashtableSize = sizeof(entry) * settings.maxTextureCnt;
    *memoryRequirement = sizeof(textureSystemState) + texturesSize + hashtableSize;
    if (state == 0){
        return;
    }
    systemPtr = state;
    systemPtr->settings = settings;

    void* stateStructMem = state;
    void* texturesMem = stateStructMem + sizeof(textureSystemState);
    void* hashtableMem = texturesMem + texturesSize;

    systemPtr->textures = texturesMem;

    hashtableCreate(sizeof(u64), settings.maxTextureCnt, hashtableMem, false, &systemPtr->textureIDs);

    for (u64 i = 0; i < systemPtr->settings.maxTextureCnt; i++){
        systemPtr->textures[i].id = INVALID_ID;
        systemPtr->textures[i].generation = INVALID_ID;
    }

    textureSystemCreateDefault();
}

void textureSystemShutdown(void* state){
    if (systemPtr){
        for(u64 i = 0; i < systemPtr->settings.maxTextureCnt; i++){
            texture* t = &systemPtr->textures[i];
            if (t->id != INVALID_ID){
                rendererDestroyTexture(t);
            }
        }
        rendererDestroyTexture(&systemPtr->defaultTexture);
        systemPtr = 0;
    }
}

texture* textureSystemTextureGetCreate(const char* name, b8 autoDelete){
    if (isNameDefault(name)){
        FWARN("Cannot make texture with default name");
    }

    u64 texID = INVALID_ID;
    b8 alreadyCreated = hashtableGetID(&systemPtr->textureIDs, name, &texID);

    if (texID == INVALID_ID){
        FERROR("Texture hashtable failed to get texture.");
    }
    

    if (alreadyCreated){
        FTRACE("CREATED TEX: %d", texID);
        systemPtr->textures[texID].generation++;
        systemPtr->textures[texID].refCnt++;
        return &systemPtr->textures[texID];
    }

    texture* t = &systemPtr->textures[texID];
    
    t->type = TEXTURE_TYPE_2D;
    if (!loadTexture(name, t)){
        FERROR("Failed to load texture with name: %s from filesystem", name);
        return 0;
    }

    t->autoDelete = autoDelete;
    t->id = texID;
    t->refCnt = 1;
    strNCpy(t->name, name, FILENAME_MAX_LENGTH);
    u64 id = texID;
    FTRACE("TexID: %u", texID);
    hashtableSet(&systemPtr->textureIDs, t->name, &texID);

    return t;
}

void textureSystemTextureRelease(const char* name){
    if (isNameDefault(name)){
        FWARN("Cant release default textures.");
        return;
    }

    u64 texID;
    b8 alreadyCreated = hashtableGetID(&systemPtr->textureIDs, name, &texID);
    if (!alreadyCreated){
        FWARN("Tried to release texture that wasn't created.");
        return;
    }
    
    // Take a copy of the name since it will be wiped out by destroy,
    // (as passed in name is generally a pointer to the actual texture's name).
    char nameDup[FILENAME_MAX_LENGTH];
    strNCpy(nameDup, name, FILENAME_MAX_LENGTH);
    u64 dupID = texID;

    texture* t = &systemPtr->textures[dupID];
    t->refCnt--;
    if (t->refCnt <= 0 && t->autoDelete){
        rendererDestroyTexture(t);
        hashtableClear(&systemPtr->textureIDs, nameDup);
        FTRACE("Released Texture: %s", nameDup);
    }
    return;
}

b8 textureSystemCreateDefault(){
    const u32 dimensions = 256;
    const u32 channelCnt = 4;
    const u32 pixelCnt = dimensions * dimensions;
    // pixels[`pixels` * `channelCnt`]
    u8 pixels[256*256*4];
    fsetMemory(pixels, 255, sizeof(u8) * pixelCnt * channelCnt);

    for (u64 row = 0; row < dimensions; ++row) {
        for (u64 col = 0; col < dimensions; ++col) {
            u64 idx = (row * dimensions) + col;
            u64 idxBitsPerPixel = idx * channelCnt;
            
            if (row % 2) {
                if (col % 2) {
                    pixels[idxBitsPerPixel + 0] = 0;
                    pixels[idxBitsPerPixel + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[idxBitsPerPixel + 0] = 0;
                    pixels[idxBitsPerPixel + 1] = 0;
                }
            }
        }
    }
    strNCpy(systemPtr->defaultTexture.name, "Fusion-Default-Texture", 23);
    systemPtr->defaultTexture.autoDelete = false;
    systemPtr->defaultTexture.width = dimensions;
    systemPtr->defaultTexture.height = dimensions;
    systemPtr->defaultTexture.channelCnt = 4;
    systemPtr->defaultTexture.hasTransparency = false;
    rendererCreateTexture(pixels, &systemPtr->defaultTexture);
    systemPtr->defaultTexture.id = INVALID_ID;
    systemPtr->defaultTexture.generation = INVALID_ID;
    return true;
}

char* extName(const char* name){
    if (strSub(name, "png")){
        return "png";
    }else if (strSub(name, "jpg")){
        return "jpg";
    }

    return "";
}

b8 loadTexture(const char* textureName, texture* t) {
    resource res;
    resourceLoad(textureName, RESOURCE_TYPE_IMAGE, &res);

    imageRS* irs = res.data;

    // Use a temporary texture to load into.
    texture temp;
    temp.width = irs->width;
    temp.height = irs->height;
    temp.channelCnt = irs->channelCnt;
    
    u64 total_size = temp.width * temp.height * temp.channelCnt;
    // Check for transparency
    b32 hasTransparency = false;
    if (temp.channelCnt == 4){
        for (u64 i = 0; i < total_size; i += temp.channelCnt) {
            u8 a = irs->pixels[i + 3];
            if (a < 255) {
                hasTransparency = true;
                break;
            }
        }
    }

    temp.autoDelete = t->autoDelete;
    temp.hasTransparency = hasTransparency;
    temp.generation = INVALID_ID;

    // Acquire internal texture resources and upload to GPU.
    rendererCreateTexture(irs->pixels, &temp);

    // Take a copy of the old texture.
    texture old = *t;

    // Assign the temp texture to the pointer.
    *t = temp;

    // Destroy the old texture.
    rendererDestroyTexture(&old);

    t->generation = 0;

    // Clean up data.
    resourceUnload(&res);
    return true;
}

texture* textureSystemGetDefault(){
    return &systemPtr->defaultTexture;
}
