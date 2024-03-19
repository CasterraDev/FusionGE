#pragma once

#include "vulkanHeader.h"

void vulkanFenceCreate(vulkanHeader* header, b8 createSignaled, vulkanFence* outFence);
void vulkanFenceDestroy(vulkanHeader* header, vulkanFence* fence);

b8 vulkanFenceWait(vulkanHeader* header, vulkanFence* fence, u64 timeout_ns);

void vulkanFenceReset(vulkanHeader* header, vulkanFence* fence);