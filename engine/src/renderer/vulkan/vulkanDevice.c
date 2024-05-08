#include "vulkanDevice.h"
#include "helpers/dinoArray.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "vulkan/vulkan_core.h"

typedef struct deviceRequirements {
    b8 graphics;
    b8 present;
    b8 compute;
    b8 transfer;
    //Dino array
    const char** deviceExts;
    b8 samplerAnisotropy;
    b8 discreteGPU;
} deviceRequirements;

typedef struct deviceQueueFamilyInfo {
    u32 graphicsFamilyIdx;
    u32 presentFamilyIdx;
    u32 computeFamilyIdx;
    u32 transferFamilyIdx;
} deviceQueueFamilyInfo;

b8 doesDeviceMeetRequirements(VkPhysicalDevice device, VkSurfaceKHR surface, const VkPhysicalDeviceProperties* properties, const VkPhysicalDeviceFeatures* features, deviceRequirements* requirements, deviceQueueFamilyInfo* outQueuesInfo, vulkanSwapchainSupportInfo* outSwapchainSupport){
    if (requirements->discreteGPU) {
        if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            FINFO("Device is not a discrete GPU, and one is required. Skipping.");
            return false;
        }
    }

    outQueuesInfo->graphicsFamilyIdx = -1;
    outQueuesInfo->presentFamilyIdx = -1;
    outQueuesInfo->computeFamilyIdx = -1;
    outQueuesInfo->transferFamilyIdx = -1;

    u32 queuesCnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuesCnt, 0);
    VkQueueFamilyProperties queues[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queuesCnt, queues);

    u8 minTransferScore = 255;
    for (u32 i = 0; i < queuesCnt; ++i) {
        u8 currentTransferScore = 0;

        //Graphics queue?
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outQueuesInfo->graphicsFamilyIdx = i;
            ++currentTransferScore;
        }

        //Compute queue?
        if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            outQueuesInfo->computeFamilyIdx = i;
            ++currentTransferScore;
        }

        //Transfer queue?
        if (queues[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            //Take the Idx if it is the current lowest. This increases the
            //liklihood that it is a dedicated transfer queue.
            if (currentTransferScore <= minTransferScore) {
                minTransferScore = currentTransferScore;
                outQueuesInfo->transferFamilyIdx = i;
            }
        }

        //Present queue?
        VkBool32 supportsPresent = VK_FALSE;
        VULKANSUCCESS(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
        if (supportsPresent) {
            outQueuesInfo->presentFamilyIdx = i;
        }
    }
    // Query swapchain support.
    vulkanDeviceQuerySwapchainSupport(
        device,
        surface,
        outSwapchainSupport);

    return true;
}

b8 createVulkanLogicalDevice(vulkanHeader* header){
    FINFO("Creating logical device...");
    // NOTE: Do not create additional queues for shared indices.
    b8 present_shares_graphics_queue = header->device.graphicsQueueIdx == header->device.presentQueueIdx;
    b8 transfer_shares_graphics_queue = header->device.graphicsQueueIdx == header->device.transferQueueIdx;
    u32 index_count = 1;
    if (!present_shares_graphics_queue) {
        index_count++;
    }
    if (!transfer_shares_graphics_queue) {
        index_count++;
    }
    u32 indices[32];
    u8 index = 0;
    indices[index++] = header->device.graphicsQueueIdx;
    if (!present_shares_graphics_queue) {
        indices[index++] = header->device.presentQueueIdx;
    }
    if (!transfer_shares_graphics_queue) {
        indices[index++] = header->device.transferQueueIdx;
    }

    VkDeviceQueueCreateInfo queue_create_infos[32];
    for (u32 i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;
        //May break some devices. TODO: Make sure the selected devices can support this
        // if (indices[i] == header->device.graphicsQueueIdx) {
        //     queue_create_infos[i].queueCount = 2;
        // }
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
        f32 queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    // Request device features.
    // TODO: should be config driven
    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;  // Request anistrophy

    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = 1;
    const char* extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    device_create_info.ppEnabledExtensionNames = &extension_names;

    // Deprecated and ignored, so pass nothing.
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = 0;

    // Create the device.
    VULKANSUCCESS(vkCreateDevice(
        header->device.physicalDevice,
        &device_create_info,
        header->allocator,
        &header->device.logicalDevice));

    FINFO("Logical device created.");

    // Get queues.
    vkGetDeviceQueue(
        header->device.logicalDevice,
        header->device.graphicsQueueIdx,
        0,
        &header->device.graphicsQueue);

    vkGetDeviceQueue(
        header->device.logicalDevice,
        header->device.presentQueueIdx,
        0,
        &header->device.presentQueue);

    vkGetDeviceQueue(
        header->device.logicalDevice,
        header->device.transferQueueIdx,
        0,
        &header->device.transferQueue);
    FINFO("Queues obtained.");

    VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.queueFamilyIndex = header->device.graphicsQueueIdx;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VULKANSUCCESS(vkCreateCommandPool(header->device.logicalDevice, &poolCreateInfo, header->allocator, &header->device.graphicsCommandPool));
    FINFO("Graphics command pool created.");

    return true;
}

b8 getVulkanDevice(vulkanHeader* header){
    u32 deviceCnt = 0;
    vkEnumeratePhysicalDevices(header->instance, &deviceCnt, 0);

    if (deviceCnt == 0) {
        FFATAL("No devices detected with Vulkan support");
        return false;
    }

    VkPhysicalDevice devices[32];
    VULKANSUCCESS(vkEnumeratePhysicalDevices(header->instance, &deviceCnt, devices));

    for (u32 i = 0; i < deviceCnt; ++i) {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memory;

        vkGetPhysicalDeviceProperties(devices[i], &properties);
        vkGetPhysicalDeviceFeatures(devices[i], &features);
        vkGetPhysicalDeviceMemoryProperties(devices[i], &memory);

        b8 supportsDeviceLocalHost = false;
        for (u32 i = 0; i < memory.memoryTypeCount; i++){
            if (((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) && 
                    ((memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
                supportsDeviceLocalHost = true;
            }
        }

        //Set the requirements needed for the app
        //TODO: Make some of these configurable options if able
        deviceRequirements requirements = {};
        requirements.graphics = true;
        requirements.present = true;
        requirements.transfer = true;
        requirements.samplerAnisotropy = true;
        requirements.discreteGPU = true;
        requirements.deviceExts = dinoCreate(const char*);
        dinoPush(requirements.deviceExts, &VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        deviceQueueFamilyInfo queueInfo = {};
        b8 isGood = doesDeviceMeetRequirements(devices[i],header->surface,&properties,&features,&requirements,&queueInfo,&header->device.swapchainSupport);
        
        if (isGood){
            FINFO("Selected device: '%s'.", properties.deviceName);

            //Print out selected GPU Info
            switch (properties.deviceType) {
                default:
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    FINFO("GPU type is Unknown.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    FINFO("GPU type is Integrated.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    FINFO("GPU type is Descrete.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    FINFO("GPU type is Virtual.");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    FINFO("GPU type is CPU.");
                    break;
            }

            FINFO(
                "GPU Driver version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion));

            //Vulkan API version.
            FINFO(
                "Vulkan API version: %d.%d.%d",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion));

            //Memory information
            for (u32 j = 0; j < memory.memoryHeapCount; ++j) {
                f32 memoryInGib = (((f32)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
                if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                    FINFO("Local GPU memory: %.2f GiB", memoryInGib);
                } else {
                    FINFO("Shared System memory: %.2f GiB", memoryInGib);
                }
            }

            //Save some info about the devices so we don't have to re-loop through them
            header->device.physicalDevice = devices[i];
            header->device.graphicsQueueIdx = queueInfo.graphicsFamilyIdx;
            header->device.presentQueueIdx = queueInfo.presentFamilyIdx;
            header->device.computeQueueIdx = queueInfo.computeFamilyIdx;
            header->device.transferQueueIdx = queueInfo.transferFamilyIdx;

            header->device.properties = properties;
            header->device.features = features;
            header->device.memory = memory;
            header->device.supportsDeviceLocalHost = supportsDeviceLocalHost;
            break;
        }
    }
    //Make sure a device was gotten
    if (!header->device.physicalDevice) {
        FERROR("No physical devices met the requirements.");
        return false;
    }

    FINFO("Physical device selected.");

    if (!createVulkanLogicalDevice(header)){
        FERROR("Could not create Logical device");
        return false;
    }

    return true;
}

void vulkanDeviceDestroy(vulkanHeader* header) {

    // Unset queues
    header->device.graphicsQueue = 0;
    header->device.presentQueue = 0;
    header->device.transferQueue = 0;

    FINFO("Destroying command pools...");
    vkDestroyCommandPool(header->device.logicalDevice, header->device.graphicsCommandPool, header->allocator);

    // Destroy logical device
    FINFO("Destroying logical device...");
    if (header->device.logicalDevice) {
        vkDestroyDevice(header->device.logicalDevice, header->allocator);
        header->device.logicalDevice = 0;
    }

    // Physical devices are not destroyed.
    FINFO("Releasing physical device resources...");
    header->device.physicalDevice = 0;

    if (header->device.swapchainSupport.formats) {
        ffree(
            header->device.swapchainSupport.formats,
            sizeof(VkSurfaceFormatKHR) * header->device.swapchainSupport.formatCnt,
            MEMORY_TAG_RENDERER);
        header->device.swapchainSupport.formats = 0;
        header->device.swapchainSupport.formatCnt = 0;
    }

    if (header->device.swapchainSupport.presentModes) {
        ffree(
            header->device.swapchainSupport.presentModes,
            sizeof(VkPresentModeKHR) * header->device.swapchainSupport.presentModeCnt,
            MEMORY_TAG_RENDERER);
        header->device.swapchainSupport.presentModes = 0;
        header->device.swapchainSupport.presentModeCnt = 0;
    }

    fzeroMemory(
        &header->device.swapchainSupport.capabilities,
        sizeof(header->device.swapchainSupport.capabilities));

    header->device.graphicsQueueIdx = -1;
    header->device.presentQueueIdx = -1;
    header->device.transferQueueIdx = -1;
}

void vulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    vulkanSwapchainSupportInfo* outSupportInfo) {
    // Surface capabilities
    VULKANSUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &outSupportInfo->capabilities));
    // Surface formats
    VULKANSUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &outSupportInfo->formatCnt,
        0));
    if (outSupportInfo->formatCnt != 0) {
        if (!outSupportInfo->formats) {
            outSupportInfo->formats = fallocate(sizeof(VkSurfaceFormatKHR) * outSupportInfo->formatCnt, MEMORY_TAG_RENDERER);
        }
        VULKANSUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &outSupportInfo->formatCnt,
            outSupportInfo->formats));
    }
    // Present modes
    VULKANSUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &outSupportInfo->presentModeCnt,
        0));
    if (outSupportInfo->presentModeCnt != 0) {
        if (!outSupportInfo->presentModes) {
            outSupportInfo->presentModes = fallocate(sizeof(VkPresentModeKHR) * outSupportInfo->presentModeCnt, MEMORY_TAG_RENDERER);
        }
        VULKANSUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &outSupportInfo->presentModeCnt,
            outSupportInfo->presentModes));
    }
}

b8 vulkanDeviceDetectDepthFormat(vulkanDevice* device) {
    // Format candidates
    const u64 candidateCnt = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidateCnt; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physicalDevice, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depthFormat = candidates[i];
            return true;
        }
    }

    return false;
}
