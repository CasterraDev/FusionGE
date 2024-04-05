#pragma once

#include "defines.h"
#include "vulkanHeader.h"

void vulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    vulkanSwapchainSupportInfo* outSupportInfo);

b8 getVulkanDevice(vulkanHeader* vh);

void vulkanDeviceDestroy(vulkanHeader* vh);

b8 vulkanDeviceDetectDepthFormat(vulkanDevice* device);