#include "vulkanFence.h"
#include "core/logger.h"

void vulkanFenceCreate(vulkanHeader* header, b8 createSignaled, vulkanFence* outFence){
    //Make sure to signal the fence if needed
    outFence->isSignaled = createSignaled;
    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (outFence->isSignaled){
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VK_CHECK(vkCreateFence(header->device.logicalDevice, &fenceCreateInfo, header->allocator, &outFence->handle));
}

void vulkanFenceDestroy(vulkanHeader* header, vulkanFence* fence){
    if (fence->handle){
        vkDestroyFence(header->device.logicalDevice, fence->handle, header->allocator);
        fence->handle = 0;
    }
    fence->isSignaled = false;
}

b8 vulkanFenceWait(vulkanHeader* header, vulkanFence* fence, u64 timeout_ns){
    if (!fence->isSignaled){
        VkResult result = vkWaitForFences(header->device.logicalDevice, 1, &fence->handle, true, timeout_ns);
        switch (result) {
            case VK_SUCCESS:
                fence->isSignaled = true;
                return true;
            case VK_TIMEOUT:
                FWARN("vkFenceWait - Timed Out");
                break;
            case VK_ERROR_DEVICE_LOST:
                FERROR("vkFenceWait - VK_ERROR_DEVICE_LOST");
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                FERROR("vkFenceWait - VK_ERROR_OUT_OF_HOST_MEMORY");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                FERROR("vkFenceWait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
                break;
            default:
                FERROR("vkFenceWait - an unknown error has occurred.");
                break;
        }
    }else{
        //If already signaled, don't wait
        return true;
    }

    return false;
}

void vulkanFenceReset(vulkanHeader* header, vulkanFence* fence){
    if (fence->isSignaled){
        VK_CHECK(vkResetFences(header->device.logicalDevice, 1, &fence->handle));
        fence->isSignaled = false;
    }
}
