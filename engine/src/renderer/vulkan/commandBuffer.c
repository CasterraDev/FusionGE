#include "commandBuffer.h"
#include "core/fmemory.h"

void vulkanCommandBufferAllocate(vulkanHeader* header, VkCommandPool pool, b8 isPrimary, vulkanCommandBuffer* outCommandBuffer){

    fzeroMemory(outCommandBuffer,sizeof(outCommandBuffer));

    VkCommandBufferAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = pool;
    allocateInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.pNext = 0;

    outCommandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    VULKANSUCCESS(vkAllocateCommandBuffers(header->device.logicalDevice, &allocateInfo, &outCommandBuffer->handle));
    outCommandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkanCommandBufferFree(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* commandBuffer){
    vkFreeCommandBuffers(header->device.logicalDevice, pool, 1, &commandBuffer->handle);
    commandBuffer->handle = 0;
    commandBuffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkanCommandBufferBegin(vulkanCommandBuffer* commandBuffer, b8 isSingleUse, b8 isRenderpassContinue, b8 isSimultaneousUse){
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = 0;
    if (isSingleUse){
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (isRenderpassContinue){
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (isSimultaneousUse){
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VULKANSUCCESS(vkBeginCommandBuffer(commandBuffer->handle, &beginInfo));
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vulkanCommandBufferEnd(vulkanCommandBuffer* commandBuffer){
    VULKANSUCCESS(vkEndCommandBuffer(commandBuffer->handle));
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vulkanCommandBufferUpdateSubmitted(vulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkanCommandBufferReset(vulkanCommandBuffer* commandBuffer){
    commandBuffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkanCommandBufferAllocateAndBeginSingleUse(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* outCommandBuffer){
    vulkanCommandBufferAllocate(header, pool, true, outCommandBuffer);
    vulkanCommandBufferBegin(outCommandBuffer, true, false, false);
}

void vulkanCommandBufferEndSingleUse(vulkanHeader* header, VkCommandPool pool, vulkanCommandBuffer* commandBuffer, VkQueue queue){
    vulkanCommandBufferEnd(commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer->handle;
    VULKANSUCCESS(vkQueueSubmit(queue, 1, &submitInfo, 0));

    VULKANSUCCESS(vkQueueWaitIdle(queue));

    vulkanCommandBufferFree(header, pool, commandBuffer);
}
