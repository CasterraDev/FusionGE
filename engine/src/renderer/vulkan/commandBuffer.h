#pragma once
#include "vulkanHeader.h"

void vulkanCommandBufferAllocate(vulkanHeader* header, VkCommandPool pool, b8 isPrimary, vulkanCommandBuffer* outCommandBuffer);
void vulkanCommandBufferFree(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* commandBuffer);

void vulkanCommandBufferBegin(vulkanCommandBuffer* commandBuffer, b8 isSingleUse, b8 isRenderpassContinue, b8 isSimultaneousUse);
void vulkanCommandBufferEnd(vulkanCommandBuffer* commandBuffer);

void vulkanCommandBufferUpdateSubmitted(vulkanCommandBuffer* commandBuffer);

void vulkanCommandBufferReset(vulkanCommandBuffer* commandBuffer);

void vulkanCommandBufferAllocateAndBeginSingleUse(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* outCommandBuffer);

void vulkanCommandBufferEndSingleUse(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* commandBuffer, VkQueue queue);
