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

typedef struct {
    uint32_t min, max;
} UInt32Range;

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

VkResult createBuffer(Device *device, VkBufferCreateInfo *bufferInfo, VkBuffer *buffer);
void destroyBuffer(Device *device, VkBuffer buffer);

VkResult allocateCommandBuffer(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, VkCommandBuffer *commandBuffer);
VkResult allocateCommandBuffers(Device *device, VkCommandPool commandPool, VkCommandBufferLevel level, size_t bufferCount, VkCommandBuffer **commandBuffers);
void freeCommandBuffers(Device *device, VkCommandPool commandPool, VkCommandBuffer *buffers, size_t count);
VkResult beginSimpleCommandBuffer(VkCommandBuffer buffer);
VkResult beginOneTimeCommandBuffer(VkCommandBuffer buffer);
VkResult beginCommandBuffer(VkCommandBuffer buffer, VkCommandBufferUsageFlags flags);
VkResult endCommandBuffer(VkCommandBuffer buffer);
VkResult resetCommandBuffer(VkCommandBuffer buffer);

void cmdBeginRenderPass(VkCommandBuffer buffer, VkRenderPassBeginInfo *beginInfo);
void cmdEndRenderPass(VkCommandBuffer buffer);
void cmdBindPipeline(VkCommandBuffer buffer, VkPipelineBindPoint bindPoint, VkPipeline pipeline);
void cmdSetViewport(VkCommandBuffer buffer, VkViewport viewport);
void cmdSetScissor(VkCommandBuffer buffer, VkRect2D scissor);
void cmdDraw(VkCommandBuffer buffer, UInt32Range vertexRange, UInt32Range instanceRange);
void cmdDrawIndexed(VkCommandBuffer buffer, UInt32Range indexRange, UInt32Range instanceRange, uint32_t vertexOffset);
void cmdBindVertexBuffers(VkCommandBuffer buffer, UInt32Range bindings, VkBuffer *vertexBuffers, VkDeviceSize *offsets);
void cmdBindIndexBuffer(VkCommandBuffer buffer, VkBuffer indexBuffer, VkDeviceSize offset, VkIndexType indexType);
void cmdCopyBuffer(VkCommandBuffer buffer, VkBuffer src, VkBuffer dst, uint32_t regionCount, VkBufferCopy *regions);

VkMemoryRequirements getBufferMemoryRequirements(Device *device, VkBuffer buffer);
VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(Device *device);
VkResult bindBufferMemory(Device *device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset);
VkResult mapMemory(Device *device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, void **ptr);
void unmapMemory(Device *device, VkDeviceMemory memory);

VkResult queueSubmit(VkQueue queue, size_t submitCount, VkSubmitInfo *submits, VkFence fence);
VkResult queuePresent(VkQueue queue, VkPresentInfoKHR *presentInfo);
VkResult queueWaitIdle(VkQueue queue);

#pragma endregion

#endif
