#ifndef SWAPCHAIN_H_
#define SWAPCHAIN_H_

#include <vulkan/vulkan.h>
#include "device_api.h"

typedef struct {
    VkSwapchainKHR swapchain;
    uint32_t imageCount;
    VkImage *images;
    VkImageView *imageViews;
    // VkFramebuffer *framebuffers;

    VkFormat format;
    VkExtent2D extent;
    VkPresentModeKHR presentMode;
} Swapchain;

VkResult createSwapChain(Device *device, Window *window, VkSurfaceKHR surface, Swapchain *swapchain);
void destroySwapChain(Device *device, Swapchain *swapchain);
VkResult acquireNextImage(Device *device, Swapchain *swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *image);

#endif
