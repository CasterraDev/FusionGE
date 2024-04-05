#pragma once

#include "core/asserts.h"

#include "math/matrixMath.h"
#include "resources/resourcesTypes.h"
#include <vulkan/vulkan.h>

#define VULKAN_MAX_GEOMETRY_COUNT 4096

#define VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT 2
#define VULKAN_OVERALL_MAX_SAMPLER_COUNT 1
#define VULKAN_OVERALL_MAX_OBJECT_COUNT 1024
#define MATERIAL_SHADER_STAGE_COUNT 2

// Checks the given expression's return value is OK.
#define VULKANSUCCESS(expr)          \
    {                                \
        FASSERT(expr == VK_SUCCESS); \
    }

typedef enum vulkanRenderPassState {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
} vulkanRenderPassState;

typedef struct vulkanRenderpass {
    VkRenderPass handle;
    vector4 renderArea;
    vector4 clearColor;

    f32 depth;
    u32 stencil;

    u8 clearFlags;
    b8 hadPrev;
    b8 hasNext;

    vulkanRenderPassState state;
} vulkanRenderpass;

typedef struct vulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 formatCnt;
    VkSurfaceFormatKHR* formats;
    u32 presentModeCnt;
    VkPresentModeKHR* presentModes;
} vulkanSwapchainSupportInfo;

typedef enum vulkanCommandBufferState {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkanCommandBufferState;

typedef struct vulkanCommandBuffer {
    VkCommandBuffer handle;

    // Command buffer state.
    vulkanCommandBufferState state;
} vulkanCommandBuffer;

typedef struct vulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    vulkanSwapchainSupportInfo swapchainSupport;
    i32 graphicsQueueIdx;
    i32 presentQueueIdx;
    i32 computeQueueIdx;
    i32 transferQueueIdx;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    VkCommandPool graphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depthFormat;
} vulkanDevice;

typedef struct vulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
    VkSampler sampler;
} vulkanImage;

typedef struct vulkanSwapchain {
    VkSurfaceFormatKHR imgFormat;
    u8 maxNumOfFramesInFlight;
    VkSwapchainKHR handle;
    u32 imageCnt;
    VkImage* images;
    VkImageView* views;
    vulkanImage depthAttachment;

    //Framebuffers used for on-screen rendering, one per frame
    VkFramebuffer framebuffers[3];
} vulkanSwapchain;

typedef struct vulkanShaderStage {
    VkShaderModule module;
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkPipelineShaderStageCreateInfo stageCreateInfo;
} vulkanShaderStage;

typedef struct vulkanPipelineConfig {
    /** @brief The name of the pipeline. Used for debugging purposes. */
    char* name;
    b8 shouldDepthTest;
    b8 isWireframe;
    /** @brief A pointer to the renderpass to associate with the pipeline. */
    vulkanRenderpass* renderpass;
    /** @brief The stride of the vertex data to be used. */
    u32 stride;
    /** @brief The number of attributes. */
    u32 attrCnt;
    /** @brief An array of attributes. */
    VkVertexInputAttributeDescription* attributes;
    /** @brief The number of descriptor set layouts. */
    u32 descriptorSetLayoutCnt;
    /** @brief An array of descriptor set layouts. */
    VkDescriptorSetLayout* descriptorSetLayouts;
    /** @brief The number of stages (vertex, fragment, etc). */
    u32 stageCnt;
    /** @brief An VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT array of stages. */
    VkPipelineShaderStageCreateInfo* stages;
    /** @brief The initial viewport configuration. */
    VkViewport viewport;
    /** @brief The initial scissor configuration. */
    VkRect2D scissor;
    /** @brief The shader flags used for creating the pipeline. */
    u32 shaderFlags;
    /** @brief The number of push constant data ranges. */
    u32 pushConstRangeCnt;
    /** @brief An array of push constant data ranges. */
    //range* pushConstantRanges;
    /** @brief Collection of topology types to be supported on this pipeline. */
    u32 topologyTypes;
} vulkanPipelineConfig;

typedef struct vulkanPipeline {
    /** @brief The internal pipeline handle. */
    VkPipeline handle;
    /** @brief The pipeline layout. */
    VkPipelineLayout pipelineLayout;
} vulkanPipeline;

typedef struct vulkanBuffer {
    u64 totalSize;
    VkBuffer buffer;
    VkBufferUsageFlagBits usageFlags;
    b8 isLocked;
    VkDeviceMemory memoryDevice;
    i32 memoryIndex;
    u32 memoryPropertyFlags;
} vulkanBuffer;

typedef struct vulkanGeometryBufferInfo {
    u32 count;
    u32 stride;
    u32 bufferOffset;
} vulkanGeometryBufferInfo;

/**
 * @brief Internal buffer data for geometry.
 */
typedef struct vulkanGeometryData {
    u32 id;
    u32 generation;
    vulkanGeometryBufferInfo vertexBufferInfo;
    vulkanGeometryBufferInfo indexBufferInfo;
} vulkanGeometryData;

typedef struct vulkanDescriptorState {
    // One per frame
    u32 generations[3];
    // One per frame
    u32 ids[3];
} vulkanDescriptorState;

//Global UBO
typedef struct sceneUBO {
    mat4 proj;
    mat4 view;
} sceneUBO;

typedef struct materialUBO {
    vector4 diffuse_color;  // 16 bytes
    vector4 v_reserved0;    // 16 bytes, reserved for future use
    vector4 v_reserved1;    // 16 bytes, reserved for future use
    vector4 v_reserved2;    // 16 bytes, reserved for future use
} materialUBO;

typedef struct vulkanOverallShaderState {
    // Per frame
    VkDescriptorSet descriptorSets[3];

    // Per descriptor
    vulkanDescriptorState descriptorStates[VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT];
} vulkanOverallShaderState;

typedef struct vulkanOverallShader {
    vulkanShaderStage stages[2];
    VkDescriptorPool globalDescriptorPool;
    VkDescriptorSetLayout globalDescriptorSetLayout;

    // One descriptor set per frame
    VkDescriptorSet globalDescriptorSets[3];

    // Global uniform object.
    sceneUBO globalUbo;

    // Global uniform buffer.
    vulkanBuffer globalUniformBuffer;

    VkDescriptorPool objectDescriptorPool;
    VkDescriptorSetLayout objectDescriptorSetLayout;
    // Object uniform buffers.
    vulkanBuffer objectUniformBuffer;
    // TODO: manage a free list of some kind here instead.
    u32 objectUniformBufferIdx;

    mapType samplerTypes[VULKAN_OVERALL_MAX_SAMPLER_COUNT];

    // TODO: make dynamic
    vulkanOverallShaderState objectStates[VULKAN_OVERALL_MAX_OBJECT_COUNT];

    vulkanPipeline pipeline;
} vulkanOverallShader;

typedef struct vulkanTextureData {
    vulkanImage image;
    VkSampler sampler;
} vulkanTextureData;

typedef struct vulkanHeader{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    vulkanDevice device;
    vulkanSwapchain swapchain;
    vulkanRenderpass mainRenderpass;
    vulkanRenderpass uiRenderpass;

    //Dino Array
    vulkanCommandBuffer* graphicsCommandBuffers;

    //Dino Array
    VkSemaphore* imageAvailSemaphores;

    //Dino Array
    VkSemaphore* queueCompleteSemaphores;

    u32 inFlightFenceCnt;
    VkFence inFlightFences[2]; // one per frame - 1

    //Holds pointers to fences which exist and are owned elsewhere, one per frame
    VkFence* imagesInFlight[3];

    u32 framebufferWidth;
    u32 framebufferHeight;

    //Current generation of framebuffer size. If it does not match framebufferSizeLastGeneration,
    //a new one should be generated.
    u64 framebufferSizeGeneration;
    //The generation of the framebuffer when it was last created. Set to framebufferSizeGeneration
    //when updated
    u64 framebufferSizeLastGeneration;

    u32 imageIdx;
    u32 currentFrame;

    b8 recreatingSwapchain;


    vulkanBuffer objectVertexBuffer;
    vulkanBuffer objectIndexBuffer;

    u64 geometryVertexOffset;
    u64 geometryIndexOffset;

    vulkanGeometryData geometries[VULKAN_MAX_GEOMETRY_COUNT];

    // World Frame buffers, one per frame
    VkFramebuffer worldFrameBuffers[3];

    i32 (*findMemoryIdx)(u32 typeFilter, u32 propertyFlags);

    #if defined(_DEBUG)
        VkDebugUtilsMessengerEXT debugMessenger;
    #endif

    f32 deltaTime;

    vulkanOverallShader materialShader;
    vulkanOverallShader uiShader;
} vulkanHeader;
