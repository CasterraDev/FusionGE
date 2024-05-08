#pragma once

#include "vulkanHeader.h"

b8 vulkanBufferCreate(vulkanHeader* header, u64 size,
                      VkBufferUsageFlagBits usage, u32 memoryPropertyFlags,
                      b8 autoBind, b8 hasFreelist, vulkanBuffer* outBuffer);

void vulkanBufferDestroy(vulkanHeader* header, vulkanBuffer* buffer);

b8 vulkanBufferAllocate(vulkanBuffer* buffer, u64 size, u64* outOffset);
b8 vulkanBufferFree(vulkanBuffer* buffer, u64 size, u64 offset);

b8 vulkanBufferResize(vulkanHeader* header, u64 newSize, vulkanBuffer* buffer,
                      VkQueue queue, VkCommandPool pool);

void vulkanBufferBind(vulkanHeader* header, vulkanBuffer* buffer, u64 offset);

void* vulkanBufferLockMemory(vulkanHeader* header, vulkanBuffer* buffer,
                             u64 offset, u64 size, u32 flags);
void vulkanBufferUnlockMemory(vulkanHeader* header, vulkanBuffer* buffer);

void vulkanBufferLoadData(vulkanHeader* header, vulkanBuffer* buffer,
                          u64 offset, u64 size, u32 flags, const void* data);

void vulkanBufferCopyTo(vulkanHeader* header, VkCommandPool pool, VkFence fence,
                        VkQueue queue, VkBuffer src, u64 srcOffset,
                        VkBuffer dest, u64 destOffset, u64 size);
