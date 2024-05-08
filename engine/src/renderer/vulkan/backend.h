#pragma once

#include "renderer/renderTypes.h"
#include "systems/shaderSystem.h"

b8 vulkanInit(struct rendererBackend* backend, const char* appName);

void vulkanShutdown(struct rendererBackend* backend);

void vulkanResized(struct rendererBackend* backend, u16 width, u16 height);
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

b8 vulkanShaderCreate(struct shader* shader, u8 renderpassID, u8 stageCnt, shaderStage* stages, const char** stageFilenames);
b8 vulkanShaderDestroy(struct shader*);
b8 vulkanShaderInit(struct shader* shader);
b8 vulkanShaderUse(struct shader* shader);
b8 vulkanShaderApplyGlobals(struct shader* shader);
b8 vulkanShaderApplyInstance(struct shader* shader);
b8 vulkanShaderBindGlobals(struct shader* shader);
b8 vulkanShaderBindInstance(struct shader* shader, u32 instanceID);
b8 vulkanShaderUniformSet(shader* s, shaderUniform u, const void* val);
b8 vulkanShaderFreeInstanceStruct(shader* s, u32 instanceID);
b8 vulkanShaderAllocateInstanceStruct(shader* s, u32* outInstanceID);
