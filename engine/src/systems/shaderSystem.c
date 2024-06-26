#include "shaderSystem.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "helpers/dinoArray.h"
#include "helpers/hashtable.h"
#include "renderer/rendererFront.h"
#include "resources/resourcesTypes.h"
#include "systems/textureSystem.h"

typedef struct shaderSystemState {
    shaderSystemSettings settings;
    // Hashtable that stores IDs for the `shaders` array so you can lookup by
    // name
    hashtable shaderIDs;
    void* hashtableMemory;
    // Array of shaders.
    shader* shaders;
    int actualShaderCnt;
    u32 curShaderID;
} shaderSystemState;

static shaderSystemState* systemPtr = 0;

b8 addUniform(shader* s, const char* uniformName, u32 size,
              shaderUniformType type, shaderScope scope, u32 location,
              b8 isSampler);

u32 getShaderID(const char* shaderName) {
    u32 id = INVALID_ID;
    if (!hashtableGet(&systemPtr->shaderIDs, shaderName, &id)) {
        FERROR("Getting shader id.");
        return INVALID_ID;
    }
    return id;
}

b8 shaderSystemInit(u64* memReq, void* memory, shaderSystemSettings settings) {
    // Check settings
    if (settings.maxShaderCnt <= 0) {
        FERROR("ShaderSystem: Max shaders must be more than 0\n");
    }

    u64 stateReq = sizeof(shaderSystemState);
    u64 hashtableReq = sizeof(entry) * settings.maxShaderCnt;
    u64 shaderArrayReq = sizeof(shader) * settings.maxShaderCnt;
    *memReq = stateReq + hashtableReq + shaderArrayReq;

    if (!memory) {
        return true;
    }

    // Give the memory blocks to the systemPtr vars
    systemPtr = memory;
    systemPtr->hashtableMemory = (void*)((u64)memory + stateReq);
    systemPtr->shaders = (void*)(systemPtr->hashtableMemory + hashtableReq);

    systemPtr->settings = settings;

    systemPtr->actualShaderCnt = 0;
    systemPtr->curShaderID = INVALID_ID;

    // Create the hashtable
    hashtableCreate(sizeof(u32), settings.maxShaderCnt,
                    systemPtr->hashtableMemory, false, 0,
                    &systemPtr->shaderIDs);


    return true;
}

b8 shaderSystemShutdown(void* state) {
    if (systemPtr) {
        // Destroy shaders
        for (u32 i = 0; i < systemPtr->settings.maxShaderCnt; i++) {
            shader* s = &systemPtr->shaders[i];
            // TODO: Destroy the shader
        }
        hashtableDestroy(&systemPtr->shaderIDs);
        fzeroMemory(systemPtr, sizeof(&systemPtr));
    }
    systemPtr = 0;
    return true;
}

b8 shaderSystemCreate(const shaderRS* config) {
    if (systemPtr->actualShaderCnt > systemPtr->settings.maxShaderCnt) {
        FERROR("Shader array is full. Booting...");
        return false;
    }

    u32 id = systemPtr->actualShaderCnt;
    systemPtr->actualShaderCnt++;

    shader* s = &systemPtr->shaders[id];
    s->id = id;
    s->state = SHADER_STATE_UN_INITED;
    s->name = strDup(config->name);
    s->hasInstances = config->hasInstances;
    s->hasLocals = config->hasLocals;
    s->pushConstRangeCnt = 0;
    fzeroMemory(s->pushConstRanges, sizeof(range) * 32);
    s->boundInstanceId = INVALID_ID;
    s->attributeStride = 0;

    // Setup DinoArrays
    s->globalTextures = dinoCreate(texture*);
    s->uniforms = dinoCreate(shaderUniform);
    s->attributes = dinoCreate(shaderAttribute);

    u64 elSize = sizeof(u16);
    u64 elCnt = 512;
    u64 memReq;
    hashtableCreate(elSize, elCnt, 0, false, &memReq, 0);
    s->uniformHashtableBlock = fallocate(sizeof(entry) * 512, MEMORY_TAG_ARRAY);
    hashtableCreate(elSize, elCnt, s->uniformHashtableBlock, false, &memReq,
                    &s->uniformsHT);

    s->globalUBOSize = 0;
    s->uniformUBOSize = 0;

    s->pushConstSize = 0;
    s->pushConstStride = 128;

    // Get renderpass id
    u8 renderpassID = 0;
    if (!rendererRenderpassID(config->renderpassName, &renderpassID)) {
        FERROR("Couldn't get renderpass ID. Booting...");
        return false;
    }

    // Have renderer create shader
    if (!rendererShaderCreate(s, renderpassID, config->stageCnt, config->stages,
                              (const char**)config->stageFilenames)) {
        FERROR("ShaderSystem: Couldn't Create Shader %s", s->name);
        return false;
    }

    // Add attributes with a loop
    for (u32 i = 0; i < config->attributeCount; i++) {
        // Get the size of the attribute
        u32 size = 0;
        switch (config->attributes[i].type) {
            case SHADER_ATTRIB_TYPE_INT8:
            case SHADER_ATTRIB_TYPE_UINT8:
                size = 1;
                break;
            case SHADER_ATTRIB_TYPE_INT16:
            case SHADER_ATTRIB_TYPE_UINT16:
                size = 2;
                break;
            case SHADER_ATTRIB_TYPE_FLOAT32:
            case SHADER_ATTRIB_TYPE_INT32:
            case SHADER_ATTRIB_TYPE_UINT32:
                size = 4;
                break;
            case SHADER_ATTRIB_TYPE_FLOAT32_2:
                size = 8;
                break;
            case SHADER_ATTRIB_TYPE_FLOAT32_3:
                size = 12;
                break;
            case SHADER_ATTRIB_TYPE_FLOAT32_4:
                size = 16;
                break;
            default:
                FERROR("Unrecognized type %d, defaulting to size of 4. This "
                       "probably is not what is desired.");
                size = 4;
                break;
        }

        s->attributeStride += size;

        shaderAttribute at = {};
        at.name = strDup(config->attributes[i].name);
        at.size = size;
        at.type = config->attributes[i].type;
        dinoPush(s->attributes, at);
    }

    // Add uniforms if sampler/uniform
    for (u32 i = 0; i < config->uniformCnt; i++) {
        if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER) {
            u32 location = 0;
            if (config->uniforms[i].scope == SHADER_SCOPE_GLOBAL) {
                u32 globalTextureCnt = dinoLength(s->globalTextures);
                if (globalTextureCnt + 1 > systemPtr->settings.maxGlobalTextures){
                    FERROR("Full global textures");
                    return false;
                }
                location = globalTextureCnt;
                dinoPush(s->globalTextures, textureSystemGetDefault());
            }else if (config->uniforms[i].scope == SHADER_SCOPE_INSTANCE){
                if (s->instanceTextureCnt + 1 > systemPtr->settings.maxInstanceTextures){
                    FERROR("Full Instance textures for shader %s.", s->name);
                    return false;
                }
                location = s->instanceTextureCnt;
                s->instanceTextureCnt++;
            }
            if (!addUniform(s, config->uniforms[i].name, 0, config->uniforms[i].type,
                       config->uniforms[i].scope, location, true)){
                FERROR("Failed to add sampler");
                return false;
            }
        } else {
            if (!addUniform(s, config->uniforms[i].name, config->uniforms[i].size,
                       config->uniforms[i].type, config->uniforms[i].scope, 0,
                       false)){
                FERROR("Failed to add uniform");
                return false;
            }
        }
    }

    // Have renderer init shader
    if (!rendererShaderInit(s)) {
        FERROR("Failed to init shader %s", s->name);
        return false;
    }

    // Set the hashtable to the id if everything is successful
    if (!hashtableSet(&systemPtr->shaderIDs, config->name, &s->id)) {
        // Destroy shader if hashtable fails
        rendererShaderDestroy(s);
    }

    return true;
}

void shaderDestroy(shader* s) {
    // Have the renderer delete the shader
    rendererShaderDestroy(s);

    s->state = SHADER_STATE_UN_INITED;
    if (s->name) {
        u32 l = strLen(s->name);
        ffree(s->name, l + 1, MEMORY_TAG_STRING);
    }
    if (s->uniformHashtableBlock != 0) {
        // TODO: Get the size of the shader to free the hashtable
        // ffree(s->uniformHashtableBlock, s. MEMORY_TAG_ARRAY);
    }
    s->name = 0;
}

void shaderSystemDestroy(const char* shaderName) {
    shader* s = shaderSystemGet(shaderName);
    if (s == 0) {
        return;
    }
    shaderDestroy(s);
}

u32 shaderSystemGetID(const char* shaderName) {
    return getShaderID(shaderName);
}

shader* shaderSystemGetByID(u32 shaderID) {
    if (shaderID >= systemPtr->settings.maxShaderCnt ||
            systemPtr->shaders[shaderID].id == INVALID_ID){
        FWARN("Shader with ID: %d. Either exceeds the `maxShaderCnt` config value or is not created yet");
        return 0;
    }
    return &systemPtr->shaders[shaderID];
}

shader* shaderSystemGet(const char* shaderName) {
    u32 id = getShaderID(shaderName);
    return (id == INVALID_ID) ? 0 : shaderSystemGetByID(id);
}

b8 shaderSystemUse(const char* shaderName) {
    u32 id = shaderSystemGetID(shaderName);
    shaderSystemUseByID(id);
    return true;
}

b8 shaderSystemUseByID(u32 shaderID) {
    if (systemPtr->curShaderID != shaderID) {
        shader* s = shaderSystemGetByID(shaderID);
        if (!rendererShaderUse(s)) {
            FERROR("Failed to use shader %s.", s->name);
            return false;
        }
        if (!rendererShaderBindGlobals(s)) {
            FERROR("Failed to bind globals for shader %s", s->name);
            return false;
        }
        systemPtr->curShaderID = shaderID;
    }
    return true;
}

u16 shaderSystemUniformIdx(shader* s, const char* uniformName) {
    if (s->id == INVALID_ID) {
        FERROR("ShaderSystemUniformIdx: Shader is not valid");
    }
    // u16 x = (u16)hashtableGetRt(&s->uniformsHT, uniformName);
    u16 x = INVALID_ID_U16;
    if (!hashtableGet(&s->uniformsHT, uniformName, &x)) {
        FERROR("ShaderSystemUniformIdx: Couldn't find uniform");
        return INVALID_ID_U16;
    }
    return s->uniforms[x].index;
}

b8 shaderSystemUniformSet(const char* uniformName, const void* val) {
    shader* s = shaderSystemGetByID(systemPtr->curShaderID);
    u16 id = INVALID_ID_U16;
    if (!hashtableGet(&s->uniformsHT, uniformName, &id)) {
        FERROR("ShaderSystemSamplerSet: Couldn't set uniform/sampler");
        return false;
    }
    return shaderSystemUniformSetByID(id, val);
}

b8 shaderSystemSamplerSet(const char* samplerName, const void* val) {
    return shaderSystemUniformSet(samplerName, val);
}

b8 shaderSystemUniformSetByID(u16 uniformID, const void* val) {
    shader* s = shaderSystemGetByID(systemPtr->curShaderID);
    shaderUniform* un = &s->uniforms[uniformID];
    if (s->boundScope != un->scope) {
        if (un->scope == SHADER_SCOPE_GLOBAL) {
            rendererShaderBindGlobals(s);
        } else if (un->scope == SHADER_SCOPE_INSTANCE) {
            rendererShaderBindInstance(s, s->boundInstanceId);
        }
        s->boundScope = un->scope;
    }
    return rendererSetUniform(s, un, val);
}

b8 shaderSystemSamplerSetByID(u16 samplerID, const void* val) {
    return shaderSystemUniformSetByID(samplerID, val);
}

b8 shaderSystemApplyGlobal() {
    return rendererShaderApplyGlobals(
        shaderSystemGetByID(systemPtr->curShaderID));
}

b8 shaderSystemApplyInstance() {
    return rendererShaderApplyInstance(
        shaderSystemGetByID(systemPtr->curShaderID));
}

b8 shaderSystemBindInstance(u32 instanceID) {
    shader* s = &systemPtr->shaders[systemPtr->curShaderID];
    s->boundInstanceId = instanceID;
    return rendererShaderBindInstance(
        shaderSystemGetByID(systemPtr->curShaderID), instanceID);
}

b8 addUniform(shader* s, const char* uniformName, u32 size,
              shaderUniformType type, shaderScope scope, u32 location,
              b8 isSampler) {
    u32 uniCnt = dinoLength(s->uniforms);
    if (uniCnt + 1 > systemPtr->settings.maxUniformCnt) {
        FERROR("UniformSampler: Uniform array is full.");
        return false;
    }

    shaderUniform un;
    un.index = uniCnt;
    un.scope = scope;
    un.type = type;
    b8 isGlobal = (scope == SHADER_SCOPE_GLOBAL);
    if (isSampler) {
        un.location = location;
    } else {
        un.location = un.index;
    }

    if (scope != SHADER_SCOPE_LOCAL) {
        un.setIdx = (u32)scope;
        un.offset = isSampler  ? 0
                    : isGlobal ? s->globalUBOSize
                               : s->uniformUBOSize;
        un.size = isSampler ? 0 : size;
    } else {
        if (un.scope == SHADER_SCOPE_LOCAL && !s->hasLocals) {
            FERROR("Cannot add local uniform to a shader that doesn't "
                   "support locals");
            return false;
        }

        un.setIdx = INVALID_ID_U8;
        range r = getAlignedRange(s->pushConstSize, size, 4);
        un.offset = r.offset;
        un.size = r.size;

        s->pushConstRanges[s->pushConstRangeCnt] = r;
        s->pushConstRangeCnt++;

        s->pushConstSize += r.size;
    }

    dinoPush(s->uniforms, un);
    FINFO("%s: %d", uniformName, s->uniforms[uniCnt].index);
    if (!hashtableSet(&s->uniformsHT, uniformName, &s->uniforms[uniCnt].index)) {
        FERROR("Failed to add uniform");
        return false;
    }

    if (!isSampler) {
        if (un.scope == SHADER_SCOPE_GLOBAL) {
            s->globalUBOSize += un.size;
        } else if (un.scope == SHADER_SCOPE_INSTANCE) {
            s->uniformUBOSize += un.size;
        }
    }

    return true;
}
