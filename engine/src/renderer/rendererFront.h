#pragma once

#include "renderTypes.h"

struct platformState;

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName);
void rendererShutdown();

void rendererOnResize(u16 width, u16 height);

b8 rendererDraw(renderHeader* header);

void rendererCreateTexture(const u8* pixels, struct texture* texture);
void rendererDestroyTexture(struct texture* texture);

/**
 * @brief Obtains the identifier of the renderpass with the given name.
 *
 * @param name The name of the renderpass whose identifier to obtain.
 * @param out_renderpass_id A pointer to hold the renderpass id.
 * @return True if found; otherwise false.
 */
b8 rendererRenderpassID(const char* name, u8* outRenderpassID);

b8 rendererCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt,
                          const void* vertices, u32 indexStride, u32 indexCnt,
                          const void* indices);

void rendererDestroyGeometry(geometry* geometry);

b8 rendererShaderCreate(shader* shader, u8 renderpassID, u8 stageCnt,
                        shaderStage* stages, const char** stageFilenames);
b8 rendererShaderDestroy(shader*);
b8 rendererShaderInit(shader* shader);
b8 rendererShaderUse(shader* shader);
b8 rendererShaderApplyGlobals(shader* shader);
b8 rendererShaderApplyInstance(shader* shader);
b8 rendererShaderBindGlobals(shader* shader);
b8 rendererShaderBindInstance(shader* shader, u32 instanceID);
b8 rendererShaderFreeInstanceStruct(shader* shader, u32 instanceID);
b8 rendererShaderAllocateInstanceStruct(shader* shader, u32* outInstanceID);

/**
 * @brief Sets the uniform of the given shader to the provided value.
 *
 * @param s A ponter to the shader.
 * @param uniform A constant pointer to the uniform.
 * @param value A pointer to the value to be set.
 * @return b8 True on success; otherwise false.
 */
b8 rendererSetUniform(struct shader* s, struct shaderUniform* uniform,
                      const void* value);
