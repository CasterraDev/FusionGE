#pragma once

#include "renderer/renderTypes.h"

b8 vulkanInit(struct rendererBackend* backend, const char* appName);

void vulkanShutdown(struct rendererBackend* backend);

void vulkanResized(struct rendererBackend* backend, u16 width, u16 height);
void vulkanUpdateGlobalState(mat4 projection, mat4 view, vector3 viewPos, vector4 ambientColor, i32 mode);
void vulkanDrawGeometry(geometryRenderData data);

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime);
b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime);

b8 vulkanCreateTexture(const u8* pixels, texture* outTexture);
void vulkanDestroyTexture(struct texture* texture);

b8 vulkanCreateMaterial(struct material* m);
void vulkanDestroyMaterial(struct material* m);

b8 vulkanCreateGeometry(struct geometry* g, u32 vertexCnt, const vertex3D* vertices, u32 indicesCnt, const u32* indices);
void vulkanDestroyGeometry(struct geometry* g);
