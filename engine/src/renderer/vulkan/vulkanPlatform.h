#pragma once

#include "defines.h"

/**
 * This is needed for the platform abstraction layer. These functions are
 * implemented in each one of the platform code files.
 * This is because Windows needs the Windows surface and Linux needs the Linux
 * Surface, This also allows you to easily require platform specific extensions
 * that are needed.
 */

struct platformState;
struct vulkanHeader;

void platformGetRequiredExts(const char*** array);

b8 platformCreateVulkanSurface(struct vulkanHeader* header);
