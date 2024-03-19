#pragma once

#include "defines.h"

struct platformState;
struct vulkanHeader;

void platformGetRequiredExts(const char*** array);

b8 platformCreateVulkanSurface(struct vulkanHeader *header);
