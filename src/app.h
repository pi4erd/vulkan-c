#ifndef APP_H_
#define APP_H_

#include <vulkan/vulkan.h>

#include "window.h"
#include "device_api.h"
#include "swapchain.h"

typedef struct VKSTATE VulkanState;

VulkanState *initVulkanState(Window *window, VkBool32 debugging);
void destroyVulkanState(VulkanState *vulkanState);
void recordCommandBuffer(VulkanState *vulkanState, uint32_t imageIndex);
VkBool32 getImage(VulkanState *vulkanState, Window *window, uint32_t *image);
void renderAndPresent(VulkanState *vulkanState, Window *window, uint32_t imageIndex);
void recreateSwapChain(VulkanState *vulkanState, Window *window);
void framebufferResized(VulkanState *vulkanState);

#endif
