#pragma once

#include "vulkanHeader.h"

void vulkanSwapchainCreate(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* outSwapchain);
void vulkanSwapchainRecreate(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* swapchain);
void vulkanSwapchainDestroy(vulkanHeader* header, vulkanSwapchain* swapchain);
b8 vulkanSwapchainGetNextImgIdx(vulkanHeader* header, vulkanSwapchain* swapchain, u64 timeoutNS, VkSemaphore imgAvailSemaphore, VkFence fence, u32* outImgIdx);
void vulkanSwapchainPresent(vulkanHeader* header, vulkanSwapchain* swapchain, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImgIdx);
