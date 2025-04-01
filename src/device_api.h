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

VkResult waitForFence(Device *device, VkFence fence, uint64_t timeout);
VkResult resetFence(Device *device, VkFence fence);
VkResult waitIdle(Device *device);

VkResult createShaderModule(Device *device, const uint32_t *code, size_t codeSize, VkShaderModule *module);
void destroyShaderModule(Device *device, VkShaderModule module);
VkResult createPipelineLayout(Device *device, VkPipelineLayoutCreateInfo *info, VkPipelineLayout *layout);
void destroyPipelineLayout(Device *device, VkPipelineLayout layout);
VkResult createRenderPass(Device *device, VkRenderPassCreateInfo *info, VkRenderPass *renderPass);
void destroyRenderPass(Device *device, VkRenderPass renderPass);
VkResult createGraphicsPipeline(Device *device, VkGraphicsPipelineCreateInfo *info, VkPipeline *pipeline);
void destroyPipeline(Device *device, VkPipeline pipeline);
VkResult createCommandPool(Device *device, uint32_t queueFamily, VkCommandPoolCreateFlags flags, VkCommandPool *commandPool);
void destroyCommandPool(Device *device, VkCommandPool commandPool);
VkResult createSemaphore(Device *device, VkSemaphore *semaphore);
void destroySemaphore(Device *device, VkSemaphore semaphore);
VkResult createFence(Device *device, VkBool32 signaled, VkFence *fence);
void destroyFence(Device *device, VkFence fence);

VkResult allocateCommandBuffer(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, VkCommandBuffer *commandBuffer);
VkResult allocateCommandBuffers(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, size_t bufferCount, VkCommandBuffer **commandBuffers);
VkResult beginSimpleCommandBuffer(VkCommandBuffer buffer);
VkResult endCommandBuffer(VkCommandBuffer buffer);
VkResult resetCommandBuffer(VkCommandBuffer buffer);

void cmdBeginRenderPass(VkCommandBuffer buffer, VkRenderPassBeginInfo *beginInfo);
void cmdEndRenderPass(VkCommandBuffer buffer);
void cmdBindPipeline(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, VkPipeline pipeline);
void cmdSetViewport(VkCommandBuffer buffer, VkViewport viewport);
void cmdSetScissor(VkCommandBuffer buffer, VkRect2D scissor);

VkResult queueSubmit(VkQueue queue, size_t submitCount, VkSubmitInfo *submits, VkFence fence);
VkResult queuePresent(VkQueue queue, VkPresentInfoKHR *presentInfo);

typedef struct {
    uint32_t min, max;
} UInt32Range;

void cmdDraw(VkCommandBuffer buffer, UInt32Range vertexRange, UInt32Range instanceRange);

#pragma endregion

#endif
