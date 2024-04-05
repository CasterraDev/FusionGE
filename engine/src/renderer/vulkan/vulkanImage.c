#include "vulkanImage.h"

#include "core/fmemory.h"
#include "core/logger.h"

void vulkanImageCreate(
    vulkanHeader* header,
    VkImageType imageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    b32 createView,
    VkImageAspectFlags viewAspectFlags,
    vulkanImage* outImg) {

    // Copy params
    outImg->width = width;
    outImg->height = height;

    // Creation info.
    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;  // TODO: Support configurable depth.
    imageCreateInfo.mipLevels = 4;     // TODO: Support mip mapping
    imageCreateInfo.arrayLayers = 1;   // TODO: Support number of layers in the image.
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Configurable sample count.
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Configurable sharing mode.

    VULKANSUCCESS(vkCreateImage(header->device.logicalDevice, &imageCreateInfo, header->allocator, &outImg->handle));

    // Query memory requirements.
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(header->device.logicalDevice, outImg->handle, &memory_requirements);

    i32 memory_type = header->findMemoryIdx(memory_requirements.memoryTypeBits, memoryFlags);
    if (memory_type == -1) {
        FERROR("Required memory type not found. Image not valid.");
    }

    // Allocate memory
    VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type;
    VULKANSUCCESS(vkAllocateMemory(header->device.logicalDevice, &memory_allocate_info, header->allocator, &outImg->memory));

    // Bind the memory
    VULKANSUCCESS(vkBindImageMemory(header->device.logicalDevice, outImg->handle, outImg->memory, 0));  // TODO: configurable memory offset.

    // Create view
    if (createView) {
        outImg->view = 0;
        vulkanImageViewCreate(header, format, outImg, viewAspectFlags);
    }
}

void vulkanImageViewCreate(
    vulkanHeader* header,
    VkFormat format,
    vulkanImage* image,
    VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.image = image->handle;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // TODO: Make configurable.
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    // TODO: Make configurable
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VULKANSUCCESS(vkCreateImageView(header->device.logicalDevice, &viewCreateInfo, header->allocator, &image->view));
}

void vulkanImageDestroy(vulkanHeader* header, vulkanImage* image) {
    vkDeviceWaitIdle(header->device.logicalDevice);
    if (image){
        if (image->view) {
            vkDestroyImageView(header->device.logicalDevice, image->view, header->allocator);
            image->view = 0;
        }
        if (image->memory) {
            vkFreeMemory(header->device.logicalDevice, image->memory, header->allocator);
            image->memory = 0;
        }
        if (image->handle) {
            vkDestroyImage(header->device.logicalDevice, image->handle, header->allocator);
            image->handle = 0;
        }
    }
}

void vulkanImageFromBuffer(vulkanHeader* header, vulkanImage* image, VkBuffer buffer, u64 offset, vulkanCommandBuffer* cb){
    VkBufferImageCopy region;
    fzeroMemory(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        cb->handle,
        buffer,
        image->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void vulkanImageTransitionLayout(vulkanHeader* header, vulkanCommandBuffer* cb, vulkanImage* image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout){
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = header->device.graphicsQueueIdx;
    barrier.dstQueueFamilyIndex = header->device.graphicsQueueIdx;
    barrier.image = image->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        FERROR("unsupported layout transition!");
        return;
    }
    
    vkCmdPipelineBarrier(
        cb->handle,
        sourceStage, destinationStage,
        0,
        0, 0,
        0, 0,
        1, &barrier
    );
}
