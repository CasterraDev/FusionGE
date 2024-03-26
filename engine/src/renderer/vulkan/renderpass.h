#pragma once

#include "vulkanHeader.h"

typedef enum renderpassClearFlags {
    RENDERPASS_CLEAR_NONE_FLAG = 0x0,
    RENDERPASS_CLEAR_COLOR_BUFFER_FLAG = 0x1,
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x2,
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x4
}renderpassClearFlags;

void vulkanRenderpassCreate(
    vulkanHeader* header, 
    vulkanRenderpass* outRenderpass,
    vector4 renderArea,
    vector4 clearColor,
    f32 depth,
    u32 stencil,
    u8 clearFlags,
    b8 hadPrev,
    b8 hasNext);

void vulkanRenderpassDestroy(vulkanHeader* header, vulkanRenderpass* renderpass);

void vulkanRenderpassBegin(
    vulkanCommandBuffer* commandBuffer, 
    vulkanRenderpass* renderpass,
    VkFramebuffer frameBuffer);

void vulkanRenderpassEnd(vulkanCommandBuffer* commandBuffer, vulkanRenderpass* renderpass);