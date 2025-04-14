#ifndef ARRAYS_H_
#define ARRAYS_H_

#include "array.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

DEFINE_ARRAY(String, const char *)
DEFINE_ARRAY(DeviceMemory, VkDeviceMemory)
DEFINE_ARRAY(UInt32, uint32_t)
DEFINE_ARRAY(PipelineStage, VkPipelineShaderStageCreateInfo)
DEFINE_ARRAY(ASGeometryBuildInfo, VkAccelerationStructureBuildGeometryInfoKHR)
DEFINE_ARRAY(ASBuildRangeInfo, VkAccelerationStructureBuildRangeInfoKHR)

#endif
