#ifndef DEVICE_UTILS_H_
#define DEVICE_UTILS_H_

#include <vulkan/vulkan.h>

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    size_t formatCount;
    VkPresentModeKHR *presentModes;
    size_t presentModeCount;
} SwapChainSupport;

VkResult querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainSupport *support);

#endif
