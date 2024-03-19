#pragma once

#include "defines.h"
#include "math/matrixMath.h"
#include "resources/resourcesTypes.h"

typedef enum renderer_backend_API {
    RENDERER_BACKEND_API_VULKAN,
    RENDERER_BACKEND_API_OPENGL,
    RENDERER_BACKEND_API_DIRECTX
} renderer_backend_API;

//Global UBO
typedef struct sceneUBO {
    mat4 proj;
    mat4 view;
} sceneUBO;

typedef struct materialUBO {
    vector4 diffuse_color;  // 16 bytes
    vector4 v_reserved0;    // 16 bytes, reserved for future use
    vector4 v_reserved1;    // 16 bytes, reserved for future use
    vector4 v_reserved2;    // 16 bytes, reserved for future use
} materialUBO;

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
    void (*drawGeometry)(geometryRenderData data);

    b8 (*beginFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*endFrame)(struct rendererBackend* backend, f32 deltaTime);
    b8 (*createTexture)(const u8* pixels, texture* outTexture);
    void (*destroyTexture)(texture* texture);

    b8 (*createMaterial)(material* m);
    void (*destroyMaterial)(material* m);

    b8 (*createGeometry)(geometry* geometry, u32 vertexCnt, const vertex3D* vertices, u32 indexCnt, const u32* indices);
    void (*destroyGeometry)(geometry* geometry);
} rendererBackend;

typedef struct renderHeader {
    u32 geometryCnt;
    geometryRenderData* geometries;
    
    f32 deltaTime;
} renderHeader;