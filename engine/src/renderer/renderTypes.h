#pragma once

#include "defines.h"
#include "math/matrixMath.h"
#include "resources/resourcesTypes.h"
#include "systems/shaderSystem.h"

typedef enum builtinRenderpass {
    BUILTIN_RENDERPASS_WORLD,
    BUILTIN_RENDERPASS_UI,
} builtinRenderpass;

typedef enum rendererBackendAPI {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_OPENGL,
    RENDERER_BACKEND_API_DIRECTX
} rendererBackendAPI;

typedef struct geometryRenderData {
    mat4 model;
    geometry* geometry;
} geometryRenderData;

typedef struct rendererBackend {
    struct platformState* pState;
    u64 frameNum;

    b8 (*init)(struct rendererBackend* backend, const char* appName);

    void (*shutdown)(struct rendererBackend* backend);

    void (*resized)(struct rendererBackend* backend, u16 width, u16 height);
    void (*drawGeometry)(geometryRenderData data);

    b8 (*beginFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*endFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*beginRenderpass)(struct rendererBackend* backend, u8 renderpassID);
    b8 (*endRenderpass)(struct rendererBackend* backend, u8 renderpassID);

    b8 (*createTexture)(const u8* pixels, texture* outTexture);
    void (*destroyTexture)(texture* texture);

    b8 (*createGeometry)(geometry* geometry, u32 vertexStride, u32 vertexCnt,
                         const void* vertices, u32 indexStride, u32 indexCnt,
                         const void* indices);
    void (*destroyGeometry)(geometry* geometry);

    b8 (*shaderCreate)(shader* shader, u8 renderpassID, u8 stageCnt,
                       shaderStage* stages, const char** stageFilenames);
    b8 (*shaderDestroy)(shader*);
    b8 (*shaderInit)(shader* shader);
    b8 (*shaderUse)(shader* shader);
    b8 (*shaderApplyGlobals)(shader* shader);
    b8 (*shaderApplyInstance)(shader* shader);
    b8 (*shaderBindGlobals)(shader* shader);
    b8 (*shaderBindInstance)(shader* shader, u32 instanceID);
    b8 (*shaderUniformSet)(shader* s, shaderUniform u, const void* val);
    b8 (*shaderFreeInstanceStruct)(shader* s, u32 instanceID);
    b8 (*shaderAllocateInstanceStruct)(shader* s, u32* outInstanceID);
} rendererBackend;

typedef struct renderHeader {
    u32 geometryCnt;
    geometryRenderData* geometries;

    u32 uiGeometryCnt;
    geometryRenderData* uiGeometries;

    f32 deltaTime;
} renderHeader;
