#ifndef DEVICE_API_H_
#define DEVICE_API_H_

#include "array.h"
#include "window.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    uint32_t graphics;
    uint32_t present;
} QueueFamilyIndices;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilies;
} Device;

VkResult createDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    VkPhysicalDeviceFeatures *features,
    StringArray layers,
    StringArray extensions,
    Device *device
);
void destroyDevice(Device *device);

void retrieveQueue(Device *device, uint32_t familyIndex, VkQueue *queue);

#pragma region API

VkResult createShaderModule(Device *device, const uint32_t *code, size_t codeSize, VkShaderModule *module);
void destroyShaderModule(Device *device, VkShaderModule module);
VkResult createPipelineLayout(Device *device, VkPipelineLayoutCreateInfo *info, VkPipelineLayout *layout);
void destroyPipelineLayout(Device *device, VkPipelineLayout layout);
VkResult createRenderPass(Device *device, VkRenderPassCreateInfo *info, VkRenderPass *renderPass);
void destroyRenderPass(Device *device, VkRenderPass renderPass);
VkResult createGraphicsPipeline(Device *device, VkGraphicsPipelineCreateInfo *info, VkPipeline *pipeline);
void destroyPipeline(Device *device, VkPipeline pipeline);

#pragma endregion

#endif
