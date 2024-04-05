#pragma once

#include "renderer/renderTypes.h"

b8 vulkanInit(struct rendererBackend* backend, const char* appName);

void vulkanShutdown(struct rendererBackend* backend);

void vulkanResized(struct rendererBackend* backend, u16 width, u16 height);
void vulkanUpdateGlobalState(mat4 projection, mat4 view, vector3 viewPos, vector4 ambientColor, i32 mode);
void vulkanUpdateUIState(mat4 projection, mat4 view, i32 mode);
void vulkanDrawGeometry(geometryRenderData data);

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime);
b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime);

b8 vulkanBeginRenderpass(struct rendererBackend* backend, u8 renderpassID);
b8 vulkanEndRenderpass(struct rendererBackend* backend, u8 renderpassID);

b8 vulkanCreateTexture(const u8* pixels, texture* outTexture);
void vulkanDestroyTexture(struct texture* texture);

b8 vulkanCreateMaterial(struct material* m);
void vulkanDestroyMaterial(struct material* m);

b8 vulkanCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt, const void* vertices, u32 indexStride, u32 indexCnt, const void* indices);
void vulkanDestroyGeometry(struct geometry* g);
