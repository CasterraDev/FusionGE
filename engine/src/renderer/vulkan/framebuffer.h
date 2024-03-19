#pragma once

#include "vulkanHeader.h"

void vulkanFramebufferCreate(
    vulkanHeader* header, vulkanRenderpass* renderpass, u32 width, u32 height, u32 attachmentCnt, VkImageView* attachments, vulkanFramebuffer* outFramebuffer);

void vulkanFramebufferDestroy(vulkanHeader* header, vulkanFramebuffer* framebuffer);