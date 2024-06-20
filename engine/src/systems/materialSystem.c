#include "materialSystem.h"
#include "textureSystem.h"
#include "helpers/hashtable.h"
#include "core/fstring.h"
#include "math/fsnmath.h"
#include "renderer/rendererFront.h"
#include "helpers/dinoArray.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "resources/resourceManager.h"

typedef struct materialSystemState
{
    hashtable materialIDs;
    void* hashtableMemory;
    material* materials;
    materialSystemSettings settings;

    material defaultMaterial;
} materialSystemState;

static materialSystemState* systemPtr;

void destroyMaterial(material* mat);

void materialSystemInit(u64* memoryRequirement, void* state, materialSystemSettings settings){
    // Block of memory will contain state structure, then block for array, then block for hashtable.
    u64 materialSize = sizeof(material) * settings.maxMaterialCnt;
    u64 hashtableSize = sizeof(entry) * settings.maxMaterialCnt;
    *memoryRequirement = sizeof(materialSystemState) + materialSize + hashtableSize;
    if (state == 0){
        return;
    }
    systemPtr = state;
    systemPtr->settings = settings;

    void* stateStructMem = state;
    void* materialsMem = stateStructMem + sizeof(materialSystemState);
    void* hashtableMem = materialsMem + materialSize;

    systemPtr->materials = materialsMem;

    hashtableCreate(sizeof(u64), settings.maxMaterialCnt, hashtableMem, false, &systemPtr->materialIDs);

    for (u64 i = 0; i < systemPtr->settings.maxMaterialCnt; i++){
        systemPtr->materials[i].id = INVALID_ID;
        systemPtr->materials[i].generation = INVALID_ID;
    }

    materialSystemCreateDefault();
}

void materialSystemShutdown(void* state){
    if (systemPtr){
        u32 c = systemPtr->settings.maxMaterialCnt;
        for (u32 i = 0; i < c; i++){
            if (systemPtr->materials[i].id != INVALID_ID){
                destroyMaterial(&systemPtr->materials[i]);
            }
        }
        //Destroy defaults
        destroyMaterial(&systemPtr->defaultMaterial);
        systemPtr = 0;
    }
}

b8 materialSystemCreateDefault(){
    fzeroMemory(&systemPtr->defaultMaterial, sizeof(material));
    systemPtr->defaultMaterial.id = INVALID_ID;
    systemPtr->defaultMaterial.generation = INVALID_ID;
    systemPtr->defaultMaterial.type = MATERIAL_TYPE_WORLD;
    systemPtr->defaultMaterial.diffuseColor = vec4One(); //White
    systemPtr->defaultMaterial.diffuseMap.type = TEXTURE_USE_MAP_DIFFUSE;
    systemPtr->defaultMaterial.diffuseMap.texture = textureSystemGetDefault();
    strCpy(systemPtr->defaultMaterial.name, DEFAULT_MATERIAL_NAME);

    if (!rendererCreateMaterial(&systemPtr->defaultMaterial)){
        FERROR("Could not make default material");
        return false;
    }

    return true;
}

material* materialSystemGetDefault(){
    return &systemPtr->defaultMaterial;
}

material* materialSystemMaterialGet(const char* name){
    if (strEqual(name, DEFAULT_MATERIAL_NAME)){
        return &systemPtr->defaultMaterial;
    }

    u64 matID;
    b8 alreadyCreated = hashtableGetID(&systemPtr->materialIDs, name, &matID);

    if (!alreadyCreated){
        material* m;
        resource res;
        if (!resourceLoad(name, RESOURCE_TYPE_MATERIAL, &res)){
            m = materialSystemGetDefault();
        }else{
            m = (material*)res.data;
        }

        resourceUnload(&res);
        
        if (!rendererCreateMaterial(m)){
            FERROR("Could not make default material");
            return false;
        }
        m->id = matID;
        systemPtr->materials[matID] = *m;
        hashtableSet(&systemPtr->materialIDs, name, &matID);
    }
    systemPtr->materials[matID].generation++;
    return &systemPtr->materials[matID];
}

void materialSystemMaterialRelease(const char* name){
    // Ignore release requests for the default material.
    if (strEqualI(name, DEFAULT_MATERIAL_NAME)) {
        return;
    }
    u64 id;
    if (systemPtr && hashtableGetID(&systemPtr->materialIDs, name, &id)) {
        material* m = &systemPtr->materials[id];
        if (m->refCnt == 0) {
            FWARN("Tried to release non-existent material: '%s'", name);
            return;
        }
        m->refCnt--;
        if (m->refCnt == 0 && m->autoDelete) {

            // Destroy/reset material.
            destroyMaterial(m);

            FTRACE("Released material '%s'., Material unloaded because reference count=0 and autoDelete=true.", name);
        }

        // Update the entry.
        hashtableClear(&systemPtr->materialIDs, name);
    } else {
        FERROR("materialSystemMaterialRelease failed to release material '%s'.", name);
    }
}

void destroyMaterial(material* mat){
    if (mat){
        rendererDestroyMaterial(mat);
        
        fzeroMemory(mat, sizeof(material));
        strNCpy(mat->name, "", 1);
        mat->id = INVALID_ID;
        mat->generation = INVALID_ID;
        mat->shaderID = INVALID_ID;
    }
}
