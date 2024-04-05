#pragma once

#include "vulkanHeader.h"

void vulkanImageCreate(
    vulkanHeader* header,
    VkImageType imageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    b32 createView,
    VkImageAspectFlags viewAspectFlags,
    vulkanImage* outImg);

void vulkanImageViewCreate(
    vulkanHeader* header,
    VkFormat format,
    vulkanImage* image,
    VkImageAspectFlags aspectFlags);

void vulkanImageDestroy(vulkanHeader* header, vulkanImage* image);

void vulkanImageFromBuffer(vulkanHeader* header, vulkanImage* image, VkBuffer buffer, u64 offset, vulkanCommandBuffer* cb);
void vulkanImageTransitionLayout(vulkanHeader* header, vulkanCommandBuffer* cb, vulkanImage* image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);