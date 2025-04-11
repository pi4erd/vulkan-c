#ifndef ARRAYS_H_
#define ARRAYS_H_

#include "array.h"
#include <vulkan/vulkan.h>

DEFINE_ARRAY(String, const char *)
DEFINE_ARRAY(DeviceMemory, VkDeviceMemory)
DEFINE_ARRAY(PipelineStage, VkPipelineShaderStageCreateInfo)

#endif
