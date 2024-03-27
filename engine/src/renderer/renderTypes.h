#pragma once

#include "defines.h"
#include "math/matrixMath.h"
#include "resources/resourcesTypes.h"

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
    void (*updateGlobalState)(mat4 projection, mat4 view, vector3 viewPos, vector4 ambientColor, i32 mode);
    void (*updateGlobalUIState)(mat4 projection, mat4 view, i32 mode);
    void (*drawGeometry)(geometryRenderData data);

    b8 (*beginFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*endFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*beginRenderpass)(struct rendererBackend* backend, u8 renderpassID);
    b8 (*endRenderpass)(struct rendererBackend* backend, u8 renderpassID);

    b8 (*createTexture)(const u8* pixels, texture* outTexture);
    void (*destroyTexture)(texture* texture);

    b8 (*createMaterial)(material* m);
    void (*destroyMaterial)(material* m);

    b8 (*createGeometry)(geometry* geometry, u32 vertexStride, u32 vertexCnt, const void* vertices, u32 indexStride, u32 indexCnt, const void* indices);
    void (*destroyGeometry)(geometry* geometry);
} rendererBackend;

typedef struct renderHeader {
    u32 geometryCnt;
    geometryRenderData* geometries;

    u32 uiGeometryCnt;
    geometryRenderData* uiGeometries;
    
    f32 deltaTime;
} renderHeader;