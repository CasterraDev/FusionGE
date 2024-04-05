#pragma once

#include "defines.h"
#include "vulkanHeader.h"

b8 vulkanPipelineCreate(vulkanHeader* header, vulkanPipelineConfig* config, vulkanPipeline* outPipeline);
void vulkanPipelineDestroy(vulkanHeader* header, vulkanPipeline* pipeline);
void vulkanPipelineBind(vulkanCommandBuffer* cb, VkPipelineBindPoint bindPoint, vulkanPipeline* pipeline);