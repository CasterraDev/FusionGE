#pragma once
#include "vulkanHeader.h"

/**
 * Returns the string representation of result.
 * @param result the vkResult to get the string for.
 * @param getExtended whether to also return an extended result
 * @returns The error code and/or extended error message in string form. Defaults to success for unknown result types.
*/
const char* vulkanResultStr(VkResult result, b8 getExtended);

/**
 * Tells if a vkResult is success or not
 * @returns True if success; otherwise false.
*/
b8 successfullVulkanResult(VkResult result);