#ifndef APP_H_
#define APP_H_

#include <vulkan/vulkan.h>

#include "window.h"
#include "device_api.h"
#include "swapchain.h"

typedef struct {
    VkInstance instance;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debugMessenger;
    Device device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    Swapchain swapchain;
} VulkanState;

VulkanState initVulkanState(Window *window, VkBool32 debugging);
void destroyVulkanState(VulkanState *vulkanState);

#endif
