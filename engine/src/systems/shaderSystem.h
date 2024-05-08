#pragma once

#include "defines.h"
#include "helpers/hashtable.h"
#include "resources/resourcesTypes.h"

typedef enum shaderState {
    SHADER_STATE_UN_INITED,
    SHADER_STATE_INITED,
} shaderState;

/**
 * @brief Represents a single entry in the internal uniform array.
 */
typedef struct shaderUniform {
    /** @brief The offset in bytes from the beginning of the uniform set (global/instance/local) */
    u64 offset;
    /**
     * @brief The location to be used as a lookup. Typically the same as the index except for samplers,
     * which is used to lookup texture index within the internal array at the given scope (global/instance).
     */
    u16 location;
    /** @brief Index into the internal uniform array. */
    u16 index;
    /** @brief The size of the uniform, or 0 for samplers. */
    u16 size;
    /** @brief The index of the descriptor set the uniform belongs to (0=global, 1=instance, INVALID_ID=local). */
    u8 setIdx;
    /** @brief The scope of the uniform. */
    shaderScope scope;
    /** @brief The type of uniform. */
    shaderUniformType type;
} shaderUniform;

/**
 * @brief Represents a single shader vertex attribute.
 */
typedef struct shaderAttribute {
    /** @brief The attribute name. */
    char* name;
    /** @brief The attribute type. */
    shaderAttributeType type;
    /** @brief The attribute size in bytes. */
    u32 size;
} shaderAttribute;

typedef struct shader {
    u32 id;
    char* name;
    b8 hasInstances;
    b8 hasLocals;
    // Some gpus have a required UBO alignment
    u64 reqUBOAlignment;

    u64 globalUBOSize;
    u64 globalUBOStride;
    u64 globalUBOOffset;

    u64 uniformUBOSize;
    u64 uniformUBOStride;

    /** @brief An array of global texture pointers. DinoArray */
    texture** globalTextures;

    u8 instanceTextureCnt;

    shaderScope boundScope;

    u64 pushConstSize;
    u64 pushConstStride;

    u8 renderpassID;

    /** @brief The block of memory used by the uniform hashtable. */
    void* uniformHashtableBlock;
    /** @brief A hashtable to store uniform index/locations by name. */
    hashtable uniformsHT;

    u8 pushConstRangeCnt;
    range pushConstRanges[32];

    /** @brief The identifier of the currently bound instance. */
    u32 boundInstanceId;
    /** @brief The currently bound instance's ubo offset. */
    u32 boundUboOffset;

    /** @brief An array of uniforms in this shader. DinoArray. */
    shaderUniform* uniforms;

    /** @brief An array of attributes. DinoArray. */
    shaderAttribute* attributes;
    /** @brief The size of all attributes combined, aka the size of a vertex. */
    u16 attributeStride;

    /** @brief The internal state of the shader. */
    shaderState state;
    // This is used to allow renderers to store render specific
    // variables
    void* internalData;
} shader;

/** @brief Configuration for the shader system. */
typedef struct shaderSystemSettings {
    /** @brief The maximum number of shaders held in the system. NOTE: Should be at least 512. */
    u16 maxShaderCnt;
    /** @brief The maximum number of uniforms allowed in a single shader. */
    u8 maxUniformCnt;
    /** @brief The maximum number of global-scope textures allowed in a single shader. */
    u8 maxGlobalTextures;
    /** @brief The maximum number of instance-scope textures allowed in a single shader. */
    u8 maxInstanceTextures;
} shaderSystemSettings;

b8 shaderSystemInit(u64* memReq, void* memory, shaderSystemSettings settings);
b8 shaderSystemShutdown(void* state);

b8 shaderSystemCreate(const shaderRS* config);
void shaderSystemDestroy(const char* shaderName);
u32 shaderSystemGetID(const char* shaderName);
shader* shaderSystemGetByID(u32 shaderID);
shader* shaderSystemGet(const char* shaderName);
b8 shaderSystemUse(const char* shaderName);
b8 shaderSystemUseByID(u32 shaderID);
u16 shaderSystemUniformIdx(shader* s, const char* uniformName);
b8 shaderSystemUniformSet(const char* uniformName, const void* val);
b8 shaderSystemSamplerSet(const char* samplerName, const void* val);
b8 shaderSystemUniformSetByID(u16 uniformID, const void* val);
b8 shaderSystemSamplerSetByID(u16 uniformID, const void* val);
b8 shaderSystemApplyGlobal();
b8 shaderSystemApplyInstance();
b8 shaderSystemBindInstance(u32 instanceID);
