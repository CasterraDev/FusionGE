#include "vulkanSwapchain.h"
#include "vulkanImage.h"
#include "core/logger.h"
#include "core/fmemory.h"
#include "vulkanDevice.h"

/**
 *  Basically translated into C code from this https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain 
 *  Also add some error catching and some performance checking.
 */

void create(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* swapchain);
void destroy(vulkanHeader* header, vulkanSwapchain* swapchain);

void vulkanSwapchainCreate(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* outSwapchain){
    create(header, width, height, outSwapchain);
}

void vulkanSwapchainRecreate(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* swapchain){
    destroy(header, swapchain);
    create(header, width, height, swapchain);
}

void vulkanSwapchainDestroy(vulkanHeader* header, vulkanSwapchain* swapchain){
    destroy(header, swapchain);
}

b8 vulkanSwapchainGetNextImgIdx(vulkanHeader* header, vulkanSwapchain* swapchain, u64 timeoutNS, VkSemaphore imgAvailSemaphore, VkFence fence, u32* outImgIdx){
    VkResult result = vkAcquireNextImageKHR(header->device.logicalDevice,swapchain->handle,timeoutNS,imgAvailSemaphore,fence,outImgIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vulkanSwapchainRecreate(header,header->framebufferWidth, header->framebufferHeight, swapchain);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
        FFATAL("Failed to acquire swapchain next image");
        return false;
    }
    return true;
}

void vulkanSwapchainPresent(vulkanHeader* header, vulkanSwapchain* swapchain, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImgIdx){
    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain->handle;
    presentInfo.pImageIndices = &presentImgIdx;
    presentInfo.pResults = 0;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        vulkanSwapchainRecreate(header,header->framebufferWidth, header->framebufferHeight, swapchain);
    } else if (result != VK_SUCCESS){
        FFATAL("Failed to acquire swapchain next image");
    }

    //Increment (and loop) the index
    header->currentFrame = (header->currentFrame + 1) % swapchain->maxNumOfFramesInFlight;
}

void create(vulkanHeader* header, u32 width, u32 height, vulkanSwapchain* swapchain){
    VkExtent2D swapchainExtent = {width,height};

    // Choose a swap surface format.
    b8 found = false;
    for (u32 i = 0; i < header->device.swapchainSupport.formatCnt; ++i) {
        VkSurfaceFormatKHR format = header->device.swapchainSupport.formats[i];
        // Preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->imgFormat = format;
            found = true;
            break;
        }
    }

    if (!found) {
        swapchain->imgFormat = header->device.swapchainSupport.formats[0];
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < header->device.swapchainSupport.presentModeCnt; ++i) {
        VkPresentModeKHR mode = header->device.swapchainSupport.presentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }

    vulkanDeviceQuerySwapchainSupport(
    header->device.physicalDevice,
    header->surface,
    &header->device.swapchainSupport);

    if (header->device.swapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        swapchainExtent = header->device.swapchainSupport.capabilities.currentExtent;
    }

    // Clamp to the values the GPU likes
    VkExtent2D min = header->device.swapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = header->device.swapchainSupport.capabilities.maxImageExtent;
    swapchainExtent.width = FCLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = FCLAMP(swapchainExtent.height, min.height, max.height);

    u32 imageCnt = header->device.swapchainSupport.capabilities.minImageCount + 1;
    if (header->device.swapchainSupport.capabilities.maxImageCount > 0 && imageCnt > header->device.swapchainSupport.capabilities.maxImageCount) {
        imageCnt = header->device.swapchainSupport.capabilities.maxImageCount;
    }

    swapchain->maxNumOfFramesInFlight = imageCnt - 1;

    // Swapchain create info
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.surface = header->surface;
    swapchainCreateInfo.minImageCount = imageCnt;
    swapchainCreateInfo.imageFormat = swapchain->imgFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchain->imgFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices
    if (header->device.graphicsQueueIdx != header->device.presentQueueIdx) {
        u32 queueFamilyIndices[] = {
            (u32)header->device.graphicsQueueIdx,
            (u32)header->device.presentQueueIdx};
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = 0;
    }

    swapchainCreateInfo.preTransform = header->device.swapchainSupport.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = 0;

    VULKANSUCCESS(vkCreateSwapchainKHR(header->device.logicalDevice, &swapchainCreateInfo, header->allocator, &swapchain->handle));

    // Start with a zero frame index.
    header->currentFrame = 0;

    // Images
    swapchain->imageCnt = 0;
    VULKANSUCCESS(vkGetSwapchainImagesKHR(header->device.logicalDevice, swapchain->handle, &swapchain->imageCnt, 0));
    if (!swapchain->images) {
        swapchain->images = (VkImage*)fallocate(sizeof(VkImage) * swapchain->imageCnt, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->views) {
        swapchain->views = (VkImageView*)fallocate(sizeof(VkImageView) * swapchain->imageCnt, MEMORY_TAG_RENDERER);
    }
    VULKANSUCCESS(vkGetSwapchainImagesKHR(header->device.logicalDevice, swapchain->handle, &swapchain->imageCnt, swapchain->images));

    // Views
    for (u32 i = 0; i < swapchain->imageCnt; ++i) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchain->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchain->imgFormat.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VULKANSUCCESS(vkCreateImageView(header->device.logicalDevice, &viewInfo, header->allocator, &swapchain->views[i]));
    }

    // Depth resources
    if (!vulkanDeviceDetectDepthFormat(&header->device)) {
        header->device.depthFormat = VK_FORMAT_UNDEFINED;
        FFATAL("Failed to find a supported format!");
    }

    // Create depth image and its view.
    vulkanImageCreate(
        header,
        VK_IMAGE_TYPE_2D,
        swapchainExtent.width,
        swapchainExtent.height,
        header->device.depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depthAttachment);

    FINFO("Swapchain created successfully.");
}

void destroy(vulkanHeader* header, vulkanSwapchain* swapchain){
    vkDeviceWaitIdle(header->device.logicalDevice);
    vulkanImageDestroy(header, &swapchain->depthAttachment);

    // Only destroy the views, not the images, since those are owned by the swapchain and are thus
    // destroyed when it is.
    for (u32 i = 0; i < swapchain->imageCnt; ++i) {
        vkDestroyImageView(header->device.logicalDevice, swapchain->views[i], header->allocator);
    }

    vkDestroySwapchainKHR(header->device.logicalDevice, swapchain->handle, header->allocator);
}
