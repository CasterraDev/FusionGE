#include "vulkanBuffers.h"
#include "commandBuffer.h"
#include "defines.h"
#include "core/logger.h"
#include "core/fmemory.h"

b8 vulkanBufferCreate(vulkanHeader* header, u64 size, VkBufferUsageFlagBits usage, u32 memoryPropertyFlags, b8 autoBind, vulkanBuffer* outBuffer){
    fzeroMemory(outBuffer, sizeof(vulkanBuffer));
    outBuffer->totalSize = size;
    outBuffer->usageFlags = usage;
    outBuffer->memoryPropertyFlags = memoryPropertyFlags;
    
    VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VULKANSUCCESS(vkCreateBuffer(header->device.logicalDevice, &bufferCreateInfo, header->allocator, &outBuffer->buffer));

    //Get the buffers memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(header->device.logicalDevice, outBuffer->buffer, &memRequirements);
    outBuffer->memoryIndex = header->findMemoryIdx(memRequirements.memoryTypeBits, outBuffer->memoryPropertyFlags);
    if (outBuffer->memoryIndex == -1) {
        FERROR("Cant create vulkan buffer because memory idx was not found");
        return false;
    }

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = (u32)outBuffer->memoryIndex;


    if (vkAllocateMemory(header->device.logicalDevice, &allocInfo, header->allocator, &outBuffer->memoryDevice) != VK_SUCCESS) {
        FERROR("Failed to allocate memory for vulkan buffers");
        return false;
    }

    if (autoBind){
        vulkanBufferBind(header, outBuffer, 0);
    }

    return true;
}

void vulkanBufferDestroy(vulkanHeader* header, vulkanBuffer* buffer){
    if (buffer->memoryDevice){
        vkFreeMemory(header->device.logicalDevice, buffer->memoryDevice, header->allocator);
        buffer->memoryDevice = 0;
    }
    if (buffer->buffer){
        vkDestroyBuffer(header->device.logicalDevice, buffer->buffer, header->allocator);
        buffer->buffer = 0;
    }
    buffer->totalSize = 0;
    buffer->usageFlags = 0;
    buffer->isLocked = false;
}

b8 vulkanBufferResize(vulkanHeader* header, u64 newSize, vulkanBuffer* buffer, VkQueue queue, VkCommandPool pool){
    VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferCreateInfo.size = newSize;
    bufferCreateInfo.usage = buffer->usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer newBuffer;
    vkCreateBuffer(header->device.logicalDevice, &bufferCreateInfo, header->allocator, &newBuffer);

    //Get the buffers memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(header->device.logicalDevice, buffer->buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = buffer->memoryIndex;

    VkDeviceMemory nbm;
    if (vkAllocateMemory(header->device.logicalDevice, &allocInfo, header->allocator, &nbm) != VK_SUCCESS) {
        FERROR("Failed to allocate memory for vulkan buffers");
        return false;
    }

    vkBindBufferMemory(header->device.logicalDevice, newBuffer, nbm, 0);

    //Copy the stuff
    vulkanBufferCopyTo(header, pool, 0, queue, buffer->buffer, 0, newBuffer, 0, buffer->totalSize);

    vkDeviceWaitIdle(header->device.logicalDevice);
    //Destroy the old buffer
    vulkanBufferDestroy(header, buffer);

    //Set all the variables
    buffer->buffer = newBuffer;
    buffer->totalSize = newSize;
    buffer->memoryDevice = nbm;
    return true;
}

void vulkanBufferBind(vulkanHeader* header, vulkanBuffer* buffer, u64 offset){
    VULKANSUCCESS(vkBindBufferMemory(header->device.logicalDevice, buffer->buffer, buffer->memoryDevice, offset));
}

void* vulkanBufferLockMemory(vulkanHeader* header, vulkanBuffer* buffer, u64 offset, u64 size, u32 flags){
    void* data;
    VULKANSUCCESS(vkMapMemory(header->device.logicalDevice, buffer->memoryDevice, offset, size, flags, &data));
    return data;
}
void vulkanBufferUnlockMemory(vulkanHeader* header, vulkanBuffer* buffer){
    vkUnmapMemory(header->device.logicalDevice, buffer->memoryDevice);
}

void vulkanBufferLoadData(vulkanHeader* header, vulkanBuffer* buffer, u64 offset, u64 size, u32 flags, const void* data){
    void* data2;
    VULKANSUCCESS(vkMapMemory(header->device.logicalDevice, buffer->memoryDevice, offset, size, flags, &data2));
    fcopyMemory(data2, data, size);
    vkUnmapMemory(header->device.logicalDevice, buffer->memoryDevice);
}

void vulkanBufferCopyTo(vulkanHeader* header, VkCommandPool pool, VkFence fence, VkQueue queue, VkBuffer src, u64 srcOffset, VkBuffer dest, u64 destOffset, u64 size){
    //Wait for the queue to finish
    vkQueueWaitIdle(queue);

    //Create a temp command pool
    vulkanCommandBuffer cb;
    vulkanCommandBufferAllocateAndBeginSingleUse(header, pool, &cb);

    VkBufferCopy vbc;
    vbc.srcOffset = srcOffset;
    vbc.dstOffset = destOffset;
    vbc.size = size;

    vkCmdCopyBuffer(cb.handle, src, dest, 1, &vbc);
    vulkanCommandBufferEndSingleUse(header, pool, &cb, queue);
}