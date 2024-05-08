#pragma once

#include "defines.h"
#include "math/matrixMath.h"

#define FILENAME_MAX_LENGTH 256
#define MATERIAL_MAX_LENGTH 256
#define TEXTURE_MAX_TEXTURES 1024
#define MAX_MATERIAL_COUNT 1024

typedef enum ResourceType {
    RESOURCE_TYPE_TEXT = 0,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_CUSTOM
} ResourceType;

typedef struct resource {
    u32 managerID;
    const char* name;
    char* fullPath;
    u32 dataSize;
    void* data;
} resource;

typedef struct imageRS {
    u8 channelCnt;
    u32 width;
    u32 height;
    u8* pixels;
} imageRS;

/** @brief Shader stages available in the system. */
typedef enum shaderStage {
    SHADER_STAGE_VERTEX = 0x00000001,
    SHADER_STAGE_GEOMETRY = 0x00000002,
    SHADER_STAGE_FRAGMENT = 0x00000004,
    SHADER_STAGE_COMPUTE = 0x0000008
} shaderStage;

/** @brief Available attribute types. */
typedef enum shaderAttributeType {
    SHADER_ATTRIB_TYPE_FLOAT32 = 0U,
    SHADER_ATTRIB_TYPE_FLOAT32_2 = 1U,
    SHADER_ATTRIB_TYPE_FLOAT32_3 = 2U,
    SHADER_ATTRIB_TYPE_FLOAT32_4 = 3U,
    SHADER_ATTRIB_TYPE_MATRIX4 = 4U,
    SHADER_ATTRIB_TYPE_INT8 = 5U,
    SHADER_ATTRIB_TYPE_UINT8 = 6U,
    SHADER_ATTRIB_TYPE_INT16 = 7U,
    SHADER_ATTRIB_TYPE_UINT16 = 8U,
    SHADER_ATTRIB_TYPE_INT32 = 9U,
    SHADER_ATTRIB_TYPE_UINT32 = 10U,
} shaderAttributeType;

/** @brief Available uniform types. */
typedef enum shaderUniformType {
    SHADER_UNIFORM_TYPE_FLOAT32 = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2 = 1U,
    SHADER_UNIFORM_TYPE_FLOAT32_3 = 2U,
    SHADER_UNIFORM_TYPE_FLOAT32_4 = 3U,
    SHADER_UNIFORM_TYPE_INT8 = 4U,
    SHADER_UNIFORM_TYPE_UINT8 = 5U,
    SHADER_UNIFORM_TYPE_INT16 = 6U,
    SHADER_UNIFORM_TYPE_UINT16 = 7U,
    SHADER_UNIFORM_TYPE_INT32 = 8U,
    SHADER_UNIFORM_TYPE_UINT32 = 9U,
    SHADER_UNIFORM_TYPE_MATRIX_4 = 10U,
    SHADER_UNIFORM_TYPE_SAMPLER = 11U,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} shaderUniformType;

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated.
 */
typedef enum shaderScope {
    /** @brief Global shader scope, generally updated once per frame. */
    SHADER_SCOPE_GLOBAL = 0,
    /** @brief Instance shader scope, generally updated "per-instance" of the
       shader. */
    SHADER_SCOPE_INSTANCE = 1,
    /** @brief Local shader scope, generally updated per-object */
    SHADER_SCOPE_LOCAL = 2
} shaderScope;

/** @brief Configuration for an attribute. */
typedef struct shaderAttributeConfig {
    /** @brief The length of the name. */
    u8 nameLength;
    /** @brief The name of the attribute. */
    char* name;
    /** @brief The size of the attribute. */
    u8 size;
    /** @brief The type of the attribute. */
    shaderAttributeType type;
} shaderAttributeConfig;

/** @brief Configuration for a uniform. */
typedef struct shaderUniformConfig {
    /** @brief The length of the name. */
    u8 nameLength;
    /** @brief The name of the uniform. */
    char* name;
    /** @brief The size of the uniform. */
    u8 size;
    /** @brief The location of the uniform. */
    u32 location;
    /** @brief The type of the uniform. */
    shaderUniformType type;
    /** @brief The scope of the uniform. */
    shaderScope scope;
} shaderUniformConfig;

/**
 * @brief Configuration for a shader. Typically created and
 * destroyed by the shader resource loader, and set to the
 * properties found in a .shadercfg resource file.
 */
typedef struct shaderRS {
    /** @brief The name of the shader to be created. */
    char* name;

    /** @brief Indicates if the shader uses instance-level uniforms. */
    b8 hasInstances;
    /** @brief Indicates if the shader uses local-level uniforms. */
    b8 hasLocals;

    /** @brief The count of attributes. */
    u8 attributeCount;
    /** @brief The collection of attributes. DinoArray. */
    shaderAttributeConfig* attributes;

    /** @brief The count of uniforms. */
    u8 uniformCnt;
    /** @brief The collection of uniforms. DinoArray. */
    shaderUniformConfig* uniforms;

    /** @brief The name of the renderpass used by this shader. */
    char* renderpassName;

    /** @brief The number of stages present in the shader. */
    u8 stageCnt;
    /** @brief The collection of stages. DinoArray. */
    shaderStage* stages;
    /** @brief The collection of stage names. Must align with stages array.
     * DinoArray. */
    char** stageNames;
    /** @brief The collection of stage file names to be loaded (one per stage).
     * Must align with stages array. DinoArray. */
    char** stageFilenames;
} shaderRS;

typedef enum textureFlags {
    /** @brief Indicates if the texture has transparency. */
    TEXTURE_FLAG_HAS_TRANSPARENCY = 0x1,
    /** @brief Indicates if the texture can be written (rendered) to. */
    TEXTURE_FLAG_IS_WRITEABLE = 0x2,
    /** @brief Indicates if the texture was created via wrapping vs traditional
       creation. */
    TEXTURE_FLAG_IS_WRAPPED = 0x4,
    /** @brief Indicates the texture is a depth texture. */
    TEXTURE_FLAG_DEPTH = 0x8
} textureFlags;

typedef enum textureType {
    /** @brief A standard two-dimensional texture. */
    TEXTURE_TYPE_2D,
    /** @brief A cube texture, used for cubemaps. */
    TEXTURE_TYPE_CUBE
} textureType;

typedef struct texture {
    /** @brief whether this texture should delete itself when it stops being
     * used. */
    b8 autoDelete;
    /** @brief Whether this texture has transparency. */
    b8 hasTransparency;
    /** @brief The unique texture identifier. */
    u32 id;
    /** @brief The texture type. */
    textureType type;
    /** @brief The texture width. */
    u32 width;
    /** @brief The texture height. */
    u32 height;
    /** @brief how many things are referencing this texture. */
    u64 refCnt;
    /** @brief The number of channels in the texture. */
    u8 channelCnt;
    /** @brief Holds various flags for this texture. */
    textureFlags flags;
    /** @brief The texture generation. Incremented every time the data is
     * reloaded. */
    u32 generation;
    /** @brief The texture name. */
    char name[FILENAME_MAX_LENGTH];
    /** @brief The raw texture data (pixels). */
    void* data;
} texture;

typedef enum mapType { TEXTURE_USE_UNKNOWN, TEXTURE_USE_MAP_DIFFUSE } mapType;

/** @brief Represents supported texture filtering modes. */
typedef enum textureFilter {
    /** @brief Nearest-neighbor filtering. */
    TEXTURE_FILTER_MODE_NEAREST,
    /** @brief Linear/bilinear filtering.*/
    TEXTURE_FILTER_MODE_LINEAR,
} textureFilter;

typedef enum textureRepeat {
    TEXTURE_REPEAT_REPEAT,
    TEXTURE_REPEAT_MIRRORED_REPEAT,
    TEXTURE_REPEAT_CLAMP_TO_EDGE,
    TEXTURE_REPEAT_CLAMP_TO_BORDER,
} textureRepeat;

typedef struct textureMap {
    texture* texture;
    mapType type;
    /** @brief Filtering mode for minifying*/
    textureFilter minFilter;
    /** @brief Filtering mode for magnifying*/
    textureFilter magFilter;
    textureRepeat repeatU;
    textureRepeat repeatV;
    textureRepeat repeatW;
    /** @brief Pointer to custom renderer data (Vulkan, OpenGL, Direct3D) so
     * they can have their own custom variables if needed*/
    void* rendererData;
} textureMap;

typedef enum MaterialTypes {
    MATERIAL_TYPE_WORLD,
    MATERIAL_TYPE_UI
} MaterialTypes;

#define DEFAULT_MATERIAL_NAME "Fusion_Default_Material"

typedef struct material {
    char name[FILENAME_MAX_LENGTH];
    /** @brief whether this material should delete itself when it stops being
     * used. */
    b8 autoDelete;
    u32 id;
    /** @brief The material generation. Incremented every time the data is
     * reloaded. */
    u32 generation;
    u32 shaderID;
    u32 instanceID;
    /** @brief how many things are referencing this material. */
    u64 refCnt;
    MaterialTypes type;

    // TODO: TEMP
    vector4 diffuseColor;
    textureMap diffuseMap;
} material;

#define GEOMETRY_NAME_MAX_LENGTH 256
#define DEFAULT_GEOMETRY_NAME "Fusion_Default_Geometry"

/**
 * @brief Represents actual geometry in the scene.
 */
typedef struct geometry {
    u32 id;
    u32 internalID;
    /** @brief The geometry generation. Incremented every time the data is
     * reloaded. */
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    material* material;
} geometry;
