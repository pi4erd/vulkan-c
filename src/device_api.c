#include "device_api.h"
#include "device_utils.h"
#include "engine.h"

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

VkResult pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, StringArray extensions, VkPhysicalDevice *physicalDevice);
VkBool32 getQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices *queueFamilies);

void retrieveQueue(Device *device, uint32_t familyIndex, VkQueue *queue) {
    vkGetDeviceQueue(device->device, familyIndex, 0, queue);
}

VkResult createDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceFeatures2 *features, StringArray layers, StringArray extensions, Device *device) {
    uint32_t result;
    result = pickPhysicalDevice(
        instance, surface,
        extensions, &device->physicalDevice
    );
    
    if(result != VK_SUCCESS) {
        return result;
    }

    if(!getQueueFamilies(device->physicalDevice, surface, &device->queueFamilies)) {
        printf("Queue families were incomplete for chosen device!\n");
        return VK_ERROR_UNKNOWN;
    }

    float queuePriority = 1.0;
    uint32_t queueCreateInfoCount = 2;
    VkDeviceQueueCreateInfo queueCreateInfos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pQueuePriorities = &queuePriority,
            .queueFamilyIndex = device->queueFamilies.graphics,
            .queueCount = 1,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pQueuePriorities = &queuePriority,
            .queueFamilyIndex = device->queueFamilies.present,
            .queueCount = 1,
        }
    };

    if(device->queueFamilies.graphics == device->queueFamilies.present)
        queueCreateInfoCount = 1;

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledExtensionNames = extensions.elements,
        .enabledExtensionCount = extensions.elementCount,
        .ppEnabledLayerNames = layers.elements,
        .enabledLayerCount = layers.elementCount,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCreateInfoCount,
        .pEnabledFeatures = VK_NULL_HANDLE,
        .pNext = features,
    };

    result = vkCreateDevice(device->physicalDevice, &createInfo, NULL, &device->device);
    if(result != VK_SUCCESS) {
        return result;
    }

    return VK_SUCCESS;
}

void destroyDevice(Device *device) {
    vkDestroyDevice(device->device, NULL);
}

VkResult waitForFence(Device *device, VkFence fence, uint64_t timeout) {
    return vkWaitForFences(device->device, 1, &fence, VK_TRUE, timeout);
}

VkResult resetFence(Device *device, VkFence fence) {
    return vkResetFences(device->device, 1, &fence);
}

VkResult waitIdle(Device *device) {
    return vkDeviceWaitIdle(device->device);
}

VkResult createShaderModule(Device *device, const uint32_t *code, size_t codeSize, VkShaderModule *module) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = code,
        .codeSize = codeSize,
    };

    return vkCreateShaderModule(device->device, &createInfo, NULL, module);
}

void destroyShaderModule(Device *device, VkShaderModule module) {
    vkDestroyShaderModule(device->device, module, NULL);
}

VkResult createPipelineLayout(Device *device, VkPipelineLayoutCreateInfo *info, VkPipelineLayout *layout) {
    return vkCreatePipelineLayout(device->device, info, NULL, layout);
}

void destroyPipelineLayout(Device *device, VkPipelineLayout layout) {
    vkDestroyPipelineLayout(device->device, layout, NULL);
}

VkResult createRenderPass(Device *device, VkRenderPassCreateInfo *info, VkRenderPass *renderPass) {
    return vkCreateRenderPass(device->device, info, NULL, renderPass);
}

void destroyRenderPass(Device *device, VkRenderPass renderPass) {
    vkDestroyRenderPass(device->device, renderPass, NULL);
}

VkResult createGraphicsPipeline(Device *device, VkGraphicsPipelineCreateInfo *info, VkPipeline *pipeline) {
    return vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, info, NULL, pipeline);
}

void destroyPipeline(Device *device, VkPipeline pipeline) {
    vkDestroyPipeline(device->device, pipeline, NULL);
}

VkResult createCommandPool(Device *device, uint32_t queueFamily, VkCommandPoolCreateFlags flags, VkCommandPool *commandPool) {
    VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queueFamily,
    };
    
    return vkCreateCommandPool(device->device, &createInfo, NULL, commandPool);
}

void destroyCommandPool(Device *device, VkCommandPool commandPool) {
    vkDestroyCommandPool(device->device, commandPool, NULL);
}

VkResult createSemaphore(Device *device, VkSemaphore *semaphore) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    return vkCreateSemaphore(device->device, &info, NULL, semaphore);
}

void destroySemaphore(Device *device, VkSemaphore semaphore) {
    vkDestroySemaphore(device->device, semaphore, NULL);
}

VkResult createFence(Device *device, VkBool32 signaled, VkFence *fence) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
    };
    return vkCreateFence(device->device, &info, NULL, fence);
}

void destroyFence(Device *device, VkFence fence) {
    vkDestroyFence(device->device, fence, NULL);
}

VkResult createAccelerationStructureKHR(
    Device *device,
    VkAccelerationStructureCreateInfoKHR *structureInfo,
    VkAccelerationStructureKHR *accelerationStructure
) {
    return VK_DEVICE_FUNC(vkCreateAccelerationStructureKHR, device->device)(
        device->device,
        structureInfo,
        NULL,
        accelerationStructure
    );
}

void destroyAccelerationStructureKHR(Device *device, VkAccelerationStructureKHR structure) {
    VK_DEVICE_FUNC(vkDestroyAccelerationStructureKHR, device->device)(device->device, structure, NULL);
}

VkResult createRayTracingPipelineKHR(
    Device *device,
    VkPipelineLayout layout,
    VkPipelineDynamicStateCreateInfo *dynamicState,
    VkRayTracingShaderGroupCreateInfoKHR *groupInfo,
    PipelineStageArray stages,
    VkPipeline *pipeline
) {
    VkRayTracingPipelineCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .layout = layout,
        .pGroups = groupInfo,
        .groupCount = 1,
        .pStages = stages.elements,
        .stageCount = stages.elementCount,
        .maxPipelineRayRecursionDepth = 8,
        .pDynamicState = dynamicState,
    };

    return VK_DEVICE_FUNC(vkCreateRayTracingPipelinesKHR, device->device)(
        device->device,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1, &info,
        NULL,
        pipeline
    );
}

VkResult createBuffer(Device *device, VkBufferCreateInfo *bufferInfo, VkBuffer *buffer) {
    return vkCreateBuffer(device->device, bufferInfo, NULL, buffer);
}

void destroyBuffer(Device *device, VkBuffer buffer) {
    vkDestroyBuffer(device->device, buffer, NULL);
}

VkResult allocateCommandBuffer(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, VkCommandBuffer *commandBuffer) {
    VkCommandBufferAllocateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = level,
        .commandBufferCount = 1,
    };

    return vkAllocateCommandBuffers(device->device, &bufferInfo, commandBuffer);
}

VkResult allocateCommandBuffers(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, size_t bufferCount, VkCommandBuffer **commandBuffers) {
    VkCommandBufferAllocateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = level,
        .commandBufferCount = bufferCount,
    };

    *commandBuffers = (VkCommandBuffer*)calloc(bufferCount, sizeof(VkCommandBuffer));

    return vkAllocateCommandBuffers(device->device, &bufferInfo, *commandBuffers);
}

void freeCommandBuffers(Device *device, VkCommandPool commandPool, VkCommandBuffer *buffers, size_t count) {
    vkFreeCommandBuffers(device->device, commandPool, count, buffers);
}

VkResult beginSimpleCommandBuffer(VkCommandBuffer buffer) {
    return beginCommandBuffer(buffer, 0);
}

VkResult beginOneTimeCommandBuffer(VkCommandBuffer buffer) {
    return beginCommandBuffer(buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

VkResult beginCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,
    };

    return vkBeginCommandBuffer(buffer, &info);
}

VkResult endCommandBuffer(VkCommandBuffer buffer) {
    return vkEndCommandBuffer(buffer);
}

VkResult resetCommandBuffer(VkCommandBuffer buffer) {
    return vkResetCommandBuffer(buffer, 0);
}

void cmdPipelineBarrier(
    VkCommandBuffer buffer,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers
) {
    vkCmdPipelineBarrier(
        buffer,
        srcStageMask, dstStageMask,
        dependencyFlags, memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers,
        imageMemoryBarrierCount, pImageMemoryBarriers
    );
}

void cmdBeginRenderPass(VkCommandBuffer buffer, VkRenderPassBeginInfo *beginInfo) {
    vkCmdBeginRenderPass(buffer, beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void cmdEndRenderPass(VkCommandBuffer buffer) {
    vkCmdEndRenderPass(buffer);
}

void cmdBindPipeline(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, VkPipeline pipeline) {
    vkCmdBindPipeline(buffer, bindPoint, pipeline);
}

void cmdSetViewport(VkCommandBuffer buffer, VkViewport viewport) {
    vkCmdSetViewport(buffer, 0, 1, &viewport);
}

void cmdSetScissor(VkCommandBuffer buffer, VkRect2D scissor) {
    vkCmdSetScissor(buffer, 0, 1, &scissor);
}

void cmdDraw(VkCommandBuffer buffer, UInt32Range vertexRange, UInt32Range instanceRange) {
    assert(vertexRange.max > vertexRange.min);
    assert(instanceRange.max > instanceRange.min);
    vkCmdDraw(
        buffer,
        vertexRange.max - vertexRange.min,
        instanceRange.max - instanceRange.min,
        vertexRange.min,
        instanceRange.min
    );
}

void cmdDrawIndexed(VkCommandBuffer buffer, UInt32Range indexRange, UInt32Range instanceRange, uint32_t vertexOffset) {
    assert(indexRange.max > indexRange.min);
    assert(instanceRange.max > instanceRange.min);
    vkCmdDrawIndexed(
        buffer,
        indexRange.max - indexRange.min,
        instanceRange.max - instanceRange.min,
        indexRange.min,
        vertexOffset,
        instanceRange.min
    );
}

void cmdBindVertexBuffers(VkCommandBuffer buffer, UInt32Range bindings, VkBuffer *vertexBuffers, VkDeviceSize *offsets) {
    assert(bindings.max > bindings.min);
    vkCmdBindVertexBuffers(
        buffer,
        bindings.min,
        bindings.max - bindings.min,
        vertexBuffers,
        offsets
    );
}

void cmdBindIndexBuffer(VkCommandBuffer buffer, VkBuffer indexBuffer, VkDeviceSize offset, VkIndexType indexType) {
    vkCmdBindIndexBuffer(
        buffer,
        indexBuffer,
        offset,
        indexType
    );
}

void cmdCopyBuffer(VkCommandBuffer buffer, VkBuffer src, VkBuffer dst, uint32_t regionCount, VkBufferCopy *regions) {
    vkCmdCopyBuffer(buffer, src, dst, regionCount, regions);
}

VkDeviceAddress getBufferAddressKHR(Device *device, VkBuffer buffer) {
    VkBufferDeviceAddressInfo addressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return VK_DEVICE_FUNC(vkGetBufferDeviceAddressKHR, device->device)(device->device, &addressInfo);
}

VkMemoryRequirements getBufferMemoryRequirements(Device *device, VkBuffer buffer) {
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device->device, buffer, &reqs);
    return reqs;
}

void getAccelerationStructureBuildSizesKHR(
    Device *device,
    VkAccelerationStructureBuildGeometryInfoKHR *geometry,
    VkAccelerationStructureBuildSizesInfoKHR *buildSizes
) {
    VK_DEVICE_FUNC(vkGetAccelerationStructureBuildSizesKHR, device->device)(
        device->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        geometry,
        NULL,
        buildSizes
    );
}

VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(Device *device) {
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &props);
    return props;
}

VkResult bindBufferMemory(Device *device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset) {
    return vkBindBufferMemory(device->device, buffer, memory, offset);
}

VkResult mapMemory(Device *device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, void **ptr) {
    return vkMapMemory(device->device, memory, offset, size, 0, ptr);
}

void unmapMemory(Device *device, VkDeviceMemory memory) {
    vkUnmapMemory(device->device, memory);
}

VkResult queueSubmit(VkQueue queue, size_t submitCount, VkSubmitInfo *submits, VkFence fence) {
    return vkQueueSubmit(queue, submitCount, submits, fence);
}

VkResult queuePresent(VkQueue queue, VkPresentInfoKHR *presentInfo) {
    return vkQueuePresentKHR(queue, presentInfo);
}

VkResult queueWaitIdle(VkQueue queue) {
    return vkQueueWaitIdle(queue);
}

void cmdBeginRenderingKHR(Device *device, VkCommandBuffer buffer, VkRenderingInfoKHR *info) {
    VK_DEVICE_FUNC(vkCmdBeginRenderingKHR, device->device)(buffer, info);
}

void cmdEndRenderingKHR(Device *device, VkCommandBuffer buffer) {
    VK_DEVICE_FUNC(vkCmdEndRenderingKHR, device->device)(buffer);
}

VkResult buildAccelerationStructuresKHR(
    Device *device,
    ASGeometryBuildInfoArray geometryInfos,
    ASBuildRangeInfoArray rangeInfos
) {
    const VkAccelerationStructureBuildRangeInfoKHR *ranges = rangeInfos.elements;
    
    VkResult result;
    result = VK_DEVICE_FUNC(vkBuildAccelerationStructuresKHR, device->device)(
        device->device,
        VK_NULL_HANDLE,
        geometryInfos.elementCount,
        geometryInfos.elements,
        &ranges
    );

    return result;
}

void cmdBuildAccelerationStructuresKHR(
    Device *device,
    VkCommandBuffer commandBuffer,
    ASGeometryBuildInfoArray geometryInfos,
    ASBuildRangeInfoArray rangeInfos
) {
    const VkAccelerationStructureBuildRangeInfoKHR *ranges = rangeInfos.elements;
    
    VK_DEVICE_FUNC(vkCmdBuildAccelerationStructuresKHR, device->device)(
        commandBuffer,
        geometryInfos.elementCount,
        geometryInfos.elements,
        &ranges
    );
}

VkBool32 getQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices *queueFamilies) {
    VkBool32 graphicsFound = VK_FALSE, presentFound = VK_FALSE;
    uint32_t graphics = UINT32_MAX, present = UINT32_MAX;

    uint32_t queueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, NULL);
    VkQueueFamilyProperties *properties = (VkQueueFamilyProperties*)calloc(queueFamiliesCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, properties);

    for(uint32_t i = 0; i < queueFamiliesCount; i++) {
        if(properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics = i;
            graphicsFound = VK_TRUE;
        }

        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentFound);

        if(presentFound) {
            present = i;
        } else {
            printf("Queue doesn't support present\n");
        }

        if(graphicsFound && presentFound) {
            break;
        }
    }

    free(properties);

    *queueFamilies = (QueueFamilyIndices){
        .graphics = graphics,
        .present = present,
    };

    return graphicsFound && presentFound;
}

VkResult pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, StringArray requiredExtensions, VkPhysicalDevice *physicalDevice) {
    uint32_t highscore = 0;
    VkPhysicalDevice bestDevice;
    VkPhysicalDeviceProperties bestProperties;

    uint32_t deviceCount;
    uint32_t result;
    
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if(result != VK_SUCCESS) {
        return result;
    }
    
    VkPhysicalDevice *devices = (VkPhysicalDevice*)calloc(deviceCount, sizeof(VkPhysicalDevice));

    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate physical devices: %s\n", string_VkResult(result));
        free(devices);
        return result;
    }

    for(uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDevice device = devices[i];
        VkBool32 deviceSuitable = VK_TRUE;

        QueueFamilyIndices queueFamilies;
        if(!getQueueFamilies(device, surface, &queueFamilies)) {
            deviceSuitable = VK_FALSE;
        }

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        (void)features;

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
        VkExtensionProperties *extensions = (VkExtensionProperties*)calloc(extensionCount, sizeof(VkExtensionProperties));
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, extensions);

        for(size_t j = 0; j < requiredExtensions.elementCount; j++) {
            VkBool32 found = VK_FALSE;
            for(uint32_t k = 0; k < extensionCount; k++) {
                if(strcmp(requiredExtensions.elements[j], extensions[k].extensionName)) {
                    found = VK_TRUE;
                    break;
                }
            }

            if(!found) {
                fprintf(stderr, "No extension present in device '%s': %s\n",
                    properties.deviceName, requiredExtensions.elements[j]);
                deviceSuitable = VK_FALSE;
            }
        }

        free(extensions);

        uint32_t score = 0;
        switch(properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 5;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 4;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 3;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 2;
            break;
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            score += 1;
            break;
            default:
            break;
        }

        SwapChainSupport support;
        querySwapChainSupport(device, surface, &support);

        deviceSuitable &= support.formatCount != 0 && support.presentModeCount != 0;

        free(support.presentModes);
        free(support.formats);

        if(deviceSuitable) {
            if(highscore < score) {
                bestDevice = device;
                bestProperties = properties;
                highscore = score;
            }
        }
    }

    if(highscore != 0) {
        fprintf(stderr, "Chosen device %s\n", bestProperties.deviceName);
        *physicalDevice = bestDevice;
    } else {
        fprintf(stderr, "No device chosen.\n");
        free(devices);
        return VK_ERROR_UNKNOWN;
    }

    free(devices);

    return VK_SUCCESS;
}
