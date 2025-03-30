#include "device_utils.h"
#include "macros.h"

#include <stdio.h>
#include <stdlib.h>

VkResult querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainSupport *support) {
    VkResult result;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &support->capabilities);
    ASSERT_ERR(result, {}, "Failed to query physical device surface capabilities.\n");

    uint32_t formatCount;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    ASSERT_ERR(result, {}, "Failed to query physical device surface format count.\n");

    if(formatCount == 0) {
        fprintf(stderr, "Physical device doesn't support any formats.\n");
        return VK_ERROR_UNKNOWN;
    }

    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR*)calloc(formatCount, sizeof(VkSurfaceFormatKHR));

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
    ASSERT_ERR(result, { free(formats); }, "Failed to query physical device surface formats.\n");

    uint32_t presentModeCount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    ASSERT_ERR(result, { free(formats); }, "Failed to query physical device surface present mode count.\n");
    
    if(presentModeCount == 0) {
        fprintf(stderr, "Physical device doesn't support any present modes.\n");
        free(formats);
        return VK_ERROR_UNKNOWN;
    }

    VkPresentModeKHR *presentModes = (VkPresentModeKHR*)calloc(presentModeCount, sizeof(VkPresentModeKHR));

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
    ASSERT_ERR(result, { free(formats); free(presentModes); }, "Failed to query physical device present modes.\n");

    *support = (SwapChainSupport){
        .capabilities = support->capabilities,
        .formats = formats,
        .formatCount = formatCount,
        .presentModes = presentModes,
        .presentModeCount = presentModeCount
    };

    return VK_SUCCESS;
}
