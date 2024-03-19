#pragma once

#include "vulkanHeader.h"

void vulkanRenderpassCreate(
    vulkanHeader* header, 
    vulkanRenderpass* outRenderpass,
    f32 x, f32 y, f32 w, f32 h,
    f32 r, f32 g, f32 b, f32 a,
    f32 depth,
    u32 stencil);

void vulkanRenderpassDestroy(vulkanHeader* header, vulkanRenderpass* renderpass);

void vulkanRenderpassBegin(
    vulkanCommandBuffer* commandBuffer, 
    vulkanRenderpass* renderpass,
    VkFramebuffer frameBuffer);

void vulkanRenderpassEnd(vulkanCommandBuffer* commandBuffer, vulkanRenderpass* renderpass);