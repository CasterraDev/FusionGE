#include "materialSystem.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoArray.h"
#include "helpers/hashtable.h"
#include "math/fsnmath.h"
#include "renderer/rendererFront.h"
#include "resources/resourceManager.h"
#include "resources/resourcesTypes.h"
#include "systems/shaderSystem.h"
#include "textureSystem.h"

// Not Pbr yet but it will be. Material shader is too confusing
typedef struct PbrUniformLocations {
    u16 projection;
    u16 view;
    u16 diffuseColor;
    u16 diffuseTexture;
    u16 model;
} PbrUniformLocations;

typedef struct uiUniformLocations {
    u16 projection;
    u16 view;
    u16 diffuseColor;
    u16 diffuseTexture;
    u16 model;
} uiUniformLocations;

typedef struct materialSystemState {
    hashtable materialIDs;
    void* hashtableMemory;
    material* materials;
    materialSystemSettings settings;

    material defaultMaterial;
    PbrUniformLocations pbrLocs;
    u32 pbrID;
    uiUniformLocations uiLocs;
    u32 uiID;
} materialSystemState;

static materialSystemState* systemPtr;

void destroyMaterial(material* mat);

b8 loadMaterial(materialFileConfig config, material* m);

void materialSystemInit(u64* memoryRequirement, void* state,
                        materialSystemSettings settings) {
    // Block of memory will contain state structure, then block for array, then
    // block for hashtable.
    u64 materialSize = sizeof(material) * settings.maxMaterialCnt;
    u64 hashtableSize = sizeof(entry) * settings.maxMaterialCnt;
    *memoryRequirement =
        sizeof(materialSystemState) + materialSize + hashtableSize;
    if (state == 0) {
        return;
    }
    systemPtr = state;
    systemPtr->settings = settings;
    systemPtr->pbrID = INVALID_ID;
    systemPtr->pbrLocs.diffuseTexture = INVALID_ID_U16;
    systemPtr->pbrLocs.diffuseColor = INVALID_ID_U16;

    systemPtr->uiID = INVALID_ID;
    systemPtr->uiLocs.diffuseTexture = INVALID_ID_U16;
    systemPtr->uiLocs.diffuseColor = INVALID_ID_U16;

    void* stateStructMem = state;
    void* materialsMem = stateStructMem + sizeof(materialSystemState);
    void* hashtableMem = materialsMem + materialSize;

    systemPtr->materials = materialsMem;

    hashtableCreate(sizeof(u64), settings.maxMaterialCnt, hashtableMem, false,
                    0, &systemPtr->materialIDs);

    for (u64 i = 0; i < systemPtr->settings.maxMaterialCnt; i++) {
        systemPtr->materials[i].id = INVALID_ID;
        systemPtr->materials[i].generation = INVALID_ID;
    }

    shader* s = shaderSystemGet(BUILTIN_MATERIAL_PBR_NAME);
    systemPtr->pbrID = s->id;
    systemPtr->pbrLocs.projection = shaderSystemUniformIdx(s, "projection");
    systemPtr->pbrLocs.view = shaderSystemUniformIdx(s, "view");
    systemPtr->pbrLocs.diffuseColor = shaderSystemUniformIdx(s, "diffuse_color");
    systemPtr->pbrLocs.diffuseTexture =
        shaderSystemUniformIdx(s, "diffuse_texture");
    systemPtr->pbrLocs.model = shaderSystemUniformIdx(s, "model");
    shader* sui = shaderSystemGet(BUILTIN_MATERIAL_UI_NAME);
    systemPtr->uiID = sui->id;
    systemPtr->uiLocs.projection = shaderSystemUniformIdx(sui, "projection");
    systemPtr->uiLocs.view = shaderSystemUniformIdx(sui, "view");
    systemPtr->uiLocs.diffuseColor = shaderSystemUniformIdx(sui, "diffuse_color");
    systemPtr->uiLocs.diffuseTexture =
        shaderSystemUniformIdx(sui, "diffuse_texture");
    systemPtr->uiLocs.model = shaderSystemUniformIdx(sui, "model");

    materialSystemCreateDefault();
}

void materialSystemShutdown(void* state) {
    if (systemPtr) {
        u32 c = systemPtr->settings.maxMaterialCnt;
        for (u32 i = 0; i < c; i++) {
            if (systemPtr->materials[i].id != INVALID_ID) {
                destroyMaterial(&systemPtr->materials[i]);
            }
        }
        // Destroy defaults
        destroyMaterial(&systemPtr->defaultMaterial);
        systemPtr = 0;
    }
}

b8 materialSystemCreateDefault() {
    fzeroMemory(&systemPtr->defaultMaterial, sizeof(material));
    systemPtr->defaultMaterial.id = INVALID_ID;
    systemPtr->defaultMaterial.generation = INVALID_ID;
    strNCpy(systemPtr->defaultMaterial.name, DEFAULT_MATERIAL_NAME,
            MATERIAL_MAX_LENGTH);
    systemPtr->defaultMaterial.diffuseColor = vec4One();
    systemPtr->defaultMaterial.diffuseMap.type = TEXTURE_USE_MAP_DIFFUSE;
    systemPtr->defaultMaterial.diffuseMap.texture = textureSystemGetDefault();
    shader* s = shaderSystemGet(BUILTIN_MATERIAL_PBR_NAME);
    if (!rendererShaderAllocateInstanceStruct(
            s, &systemPtr->defaultMaterial.instanceID)) {
        FFATAL("Failed to make default material");
        return false;
    }
    return true;
}

material* materialSystemGetDefault() {
    return &systemPtr->defaultMaterial;
}

material* materialSystemMaterialGet(const char* name) {
    if (strEqual(name, DEFAULT_MATERIAL_NAME)) {
        return &systemPtr->defaultMaterial;
    }
    resource matRes;
    if (!resourceLoad(name, RESOURCE_TYPE_MATERIAL, &matRes)){
        FERROR("Failed to load material: %s.", name);
        return false;
    }
    materialFileConfig* config = (materialFileConfig*)matRes.data;
    resourceUnload(&matRes);
    return materialSystemMaterialGetByConfig(*config);
}

material* materialSystemMaterialGetByConfig(materialFileConfig config){
    u64 matID;
    b8 alreadyCreated = hashtableGetID(&systemPtr->materialIDs, config.name, &matID);
    material* m = &systemPtr->materials[matID];

    if (!alreadyCreated) {
        loadMaterial(config, m);
        m->id = matID;
        hashtableSet(&systemPtr->materialIDs, config.name, &matID);
    }

    systemPtr->materials[matID].generation++;
    return &systemPtr->materials[matID];
} 

b8 loadMaterial(materialFileConfig config, material* m){
    fzeroMemory(m, sizeof(material));
    strNCpy(m->name, config.name, MATERIAL_MAX_LENGTH);
    m->shaderID = config.shaderID;
    m->diffuseColor = config.diffuseColor;

    if (strLen(config.DiffuseMapName) > 0){
        m->diffuseMap.type = TEXTURE_USE_MAP_DIFFUSE;
        m->diffuseMap.texture = textureSystemTextureGetCreate(config.DiffuseMapName, true);
        if (!m->diffuseMap.texture){
            FWARN("Unable to load texture: %s in material %s. Using default texture instead.", config.DiffuseMapName, m->name);
            m->diffuseMap.texture = textureSystemGetDefault();
        }
    }else{
        m->diffuseMap.type = TEXTURE_USE_UNKNOWN;
        m->diffuseMap.texture = 0;
    }
    shader* s = shaderSystemGetByID(m->shaderID);
    if (!rendererShaderAllocateInstanceStruct(s, &m->instanceID)) {
        FERROR("Unable to acquire instance resources");
        return 0;
    }

    return true;
}

void materialSystemMaterialRelease(const char* name) {
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

            FTRACE("Released material '%s'., Material unloaded because "
                   "reference count=0 and autoDelete=true.",
                   name);
        }

        // Update the entry.
        hashtableClear(&systemPtr->materialIDs, name);
    } else {
        FERROR("materialSystemMaterialRelease failed to release material '%s'.",
               name);
    }
}

void destroyMaterial(material* mat) {
    if (mat) {
        rendererShaderFreeInstanceStruct(shaderSystemGetByID(mat->shaderID),
                                         mat->instanceID);

        fzeroMemory(mat, sizeof(material));
        strNCpy(mat->name, "", 1);
        mat->id = INVALID_ID;
        mat->generation = INVALID_ID;
        mat->shaderID = INVALID_ID;
    }
}

b8 materialSystemUpdateGlobal(u32 shaderID, const mat4* proj,
                              const mat4* view) {
    if (shaderID == systemPtr->pbrID) {
        shaderSystemUniformSetByID(systemPtr->pbrLocs.projection, proj);
        shaderSystemUniformSetByID(systemPtr->pbrLocs.view, view);
    } else if (shaderID == systemPtr->uiID) {
        shaderSystemUniformSetByID(systemPtr->uiLocs.projection, proj);
        shaderSystemUniformSetByID(systemPtr->uiLocs.view, view);
    } else {
        FERROR("Material System: Unknown shaderID used");
        return false;
    }
    return shaderSystemApplyGlobal();
}

b8 materialSystemUpdateInstance(material* m) {
    shaderSystemBindInstance(m->instanceID);
    if (m->shaderID == systemPtr->pbrID) {
        shaderSystemUniformSetByID(systemPtr->pbrLocs.diffuseColor,
                                   &m->diffuseColor);
        shaderSystemUniformSetByID(systemPtr->pbrLocs.diffuseTexture,
                                   &m->diffuseMap.texture);
    } else if (m->shaderID == systemPtr->uiID) {
        shaderSystemUniformSetByID(systemPtr->uiLocs.diffuseColor,
                                   &m->diffuseColor);
        shaderSystemUniformSetByID(systemPtr->uiLocs.diffuseTexture,
                                   &m->diffuseMap.texture);
    } else {
        FERROR("Material System: Unknown shaderID used");
        return false;
    }
    return shaderSystemApplyInstance();
}

b8 materialSystemUpdateLocal(material* m, const mat4* model) {
    if (m->shaderID == systemPtr->pbrID) {
        shaderSystemUniformSetByID(systemPtr->pbrLocs.model, model);
    } else if (m->shaderID == systemPtr->uiID) {
        shaderSystemUniformSetByID(systemPtr->uiLocs.model, model);
    } else {
        FERROR("Material System: Unknown shaderID used");
        return false;
    }
    return true;
}
