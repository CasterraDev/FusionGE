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
    shader* curShader;
} shaderSystemState;

static shaderSystemState* systemPtr = 0;

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
    u64 hashtableReq = sizeof(u32) * settings.maxShaderCnt;
    u64 shaderArrayReq = sizeof(shader) * settings.maxShaderCnt;
    *memReq = stateReq + hashtableReq + shaderArrayReq;

    if (!memory) {
        return true;
    }

    // Give the memory blocks to the systemPtr vars
    systemPtr = memory;
    systemPtr->hashtableMemory = (void*)(memory + stateReq);
    systemPtr->shaders = (void*)(systemPtr->hashtableMemory + hashtableReq);

    systemPtr->settings = settings;

    systemPtr->actualShaderCnt = 0;

    // Create the hashtable
    hashtableCreate(sizeof(u32), settings.maxShaderCnt,
                    systemPtr->hashtableMemory, false, &systemPtr->shaderIDs);

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
    s->uniformHashtableBlock = fallocate(elSize * elCnt, MEMORY_TAG_UNKNOWN);
    hashtableCreate(elSize, elCnt, s->uniformHashtableBlock, false,
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
        // Check to make sure uniform isn't inited
        if (s->state != SHADER_STATE_UN_INITED) {
            FERROR("You can only add uniforms to shaders that haven't been "
                   "inited.");
            return false;
        }

        // Check if name is already used
        u16 x;
        if (hashtableGet(&s->uniformsHT, config->uniforms[i].name, &x)) {
            FERROR("Uniform: '%s' already exists on Shader: '%s'",
                   config->uniforms[i].name, s->name);
            return false;
        }
        if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER) {
            // Add it as a sampler

            if (config->uniforms[i].scope == SHADER_SCOPE_INSTANCE &&
                !s->hasInstances) {
                FERROR("Cannot add sampler with scope instance to a shader "
                       "that doesn't support instances");
                return false;
            }

            if (config->uniforms[i].scope == SHADER_SCOPE_LOCAL) {
                FERROR("Samples cannot be added to push consts");
                return false;
            }

            u32 location = 0;
            // If global push to global texture list
            // Set location to globalTextureCnt
            if (config->uniforms[i].scope == SHADER_SCOPE_GLOBAL) {
                location = dinoLength(s->globalTextures);
                // Make sure theirs room in the global array
                if (location + 1 > systemPtr->settings.maxGlobalTextures) {
                    FERROR("Shader's global texture array is full");
                    return false;
                }
                // Just push a default texture for now
                dinoPush(s->globalTextures, textureSystemGetDefault());
            } else {
                if (s->instanceTextureCnt + 1 >
                    systemPtr->settings.maxInstanceTextures) {
                    FERROR("Shader instance array is full");
                    return false;
                }
                location = s->instanceTextureCnt;
                s->instanceTextureCnt++;
            }

            u32 uniCnt = dinoLength(config->uniforms);
            if (uniCnt + 1 > systemPtr->settings.maxUniformCnt) {
                FERROR("Uniform array is full.");
                return false;
            }

            shaderUniform un;
            un.index = uniCnt;
            un.scope = config->uniforms[i].scope;
            un.type = config->uniforms[i].type;
            un.setIdx = un.scope;
            un.offset = 0;
            un.size = 0;

            if (!hashtableSet(&s->uniformsHT, config->uniforms[i].name,
                              &un.index)) {
                FERROR("Failed to add uniform");
                return false;
            }
            dinoPush(s->uniforms, un);
        } else {
            // Add it as a uniform

            // Make sure theirs room in the array for more uniforms
            u32 uniCnt = dinoLength(config->uniforms);
            if (uniCnt + 1 > systemPtr->settings.maxUniformCnt) {
                FERROR("Uniform array is full.");
                return false;
            }

            shaderUniform un;
            un.index = uniCnt;
            un.scope = config->uniforms[i].scope;
            un.type = config->uniforms[i].type;
            un.location = uniCnt;

            if (config->uniforms[i].scope != SHADER_SCOPE_LOCAL) {
                if (un.scope == SHADER_SCOPE_GLOBAL) {
                    un.offset = s->globalUBOSize;
                } else {
                    un.offset = s->uniformUBOSize;
                }
                un.size = config->uniforms[i].size;
                un.setIdx = (u32)config->uniforms[i].scope;
            } else {
                if (!s->hasLocals) {
                    FERROR("Cannot add local uniform to a shader that doesn't "
                           "support locals");
                    return false;
                }
                un.setIdx = INVALID_ID_U8;
                // Push a new aligned range (align to 4 cuz of vulkan spec)
                range r = getAlignedRange(s->pushConstSize,
                                          config->uniforms[i].size, 4);
                un.offset = r.offset;
                un.size = r.size;

                s->pushConstRanges[s->pushConstRangeCnt] = r;
                s->pushConstRangeCnt++;

                s->pushConstSize += r.size;
            }

            if (!hashtableSet(&s->uniformsHT, config->uniforms[i].name,
                              &un.index)) {
                FERROR("Failed to add uniform to HT");
                return false;
            }
            dinoPush(s->uniforms, un);
            if (un.scope == SHADER_SCOPE_GLOBAL) {
                s->globalUBOSize += un.size;
            } else if (un.scope == SHADER_SCOPE_INSTANCE) {
                s->uniformUBOSize += un.size;
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
    return (shaderID >= systemPtr->settings.maxShaderCnt ||
            systemPtr->shaders[shaderID].id == INVALID_ID)
               ? 0
               : &systemPtr->shaders[shaderID];
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
    shader* s = shaderSystemGetByID(shaderID);
    if (!rendererShaderUse(s)){
        FERROR("Failed to use shader %s.", s->name);
        return false;
    }
    if (!rendererShaderBindGlobals(s)){
        FERROR("Failed to bind globals for shader %s", s->name);
        return false;
    }
    return true;
}

u16 shaderSystemUniformIdx(shader* s, const char* uniformName){
    if (s->id == INVALID_ID){
        FERROR("ShaderSystemUniformIdx: Shader is not valid");
    }
    u16 x = INVALID_ID_U16;
    if (!hashtableGet(&s->uniformsHT, uniformName, &x)){
        FERROR("ShaderSystemUniformIdx: Couldn't find uniform");
        return INVALID_ID_U16;
    }
    return s->uniforms[x].index;
}

b8 shaderSystemUniformSet(const char* uniformName, const void* val){
    shader* s = systemPtr->curShader;
    u16 id = INVALID_ID_U16;
    if (!hashtableGet(&s->uniformsHT, uniformName, &id)){
        FERROR("ShaderSystemSamplerSet: Couldn't set uniform/sampler");
        return false;
    }
    return true;
}

b8 shaderSystemSamplerSet(const char* samplerName, const void* val){
    return shaderSystemUniformSet(samplerName, val);
}

b8 shaderSystemUniformSetByID(u16 uniformID, const void* val){
    shader* s = systemPtr->curShader;
    shaderUniform* un = &s->uniforms[uniformID];
    if (s->boundScope != un->scope){
        if (un->scope == SHADER_SCOPE_GLOBAL){
            rendererShaderBindGlobals(s);
        } else if (un->scope == SHADER_SCOPE_INSTANCE){
            rendererShaderBindInstance(s, s->boundInstanceId);
        }
        s->boundScope = un->scope;
    }
    return rendererSetUniform(s, un, val);
}

b8 shaderSystemSamplerSetByID(u16 samplerID, const void* val){
    return shaderSystemUniformSetByID(samplerID, val);
}

b8 shaderSystemApplyGlobal() {
    return rendererShaderApplyGlobals(systemPtr->curShader);
}

b8 shaderSystemApplyInstance() {
    return rendererShaderApplyInstance(systemPtr->curShader);
}

b8 shaderSystemBindInstance(u32 instanceID) {
    return rendererShaderBindInstance(systemPtr->curShader, instanceID);
}
