#include "swapchain.h"
#include "macros.h"
#include "device_utils.h"
#include "vulkan/vk_enum_string_helper.h"

#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

VkSurfaceFormatKHR chooseSurfaceFormat(VkSurfaceFormatKHR *formats, size_t formatCount);
VkPresentModeKHR choosePresentMode(VkPresentModeKHR *presentModes, size_t presentModeCount);
VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR *caps, Window *window);
uint32_t clamp(uint32_t, uint32_t, uint32_t);

VkResult acquireSwapChainImages(Device *device, VkSwapchainKHR swapchain, uint32_t *imageCount, VkImage **images);
VkResult createImageViews(Device *device, Swapchain *swapchain);

VkResult createSwapChain(Device *device, Window *window, VkSurfaceKHR surface, Swapchain *swapchain) {
    SwapChainSupport support;
    VkResult result;

    result = querySwapChainSupport(device->physicalDevice, surface, &support);
    ASSERT_ERR(result, { free(support.presentModes); free(support.formats); }, "Failed to query swap chain support.\n");

    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats, support.formatCount);
    VkPresentModeKHR presentMode = choosePresentMode(support.presentModes, support.presentModeCount);
    VkExtent2D extent = chooseExtent(&support.capabilities, window);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if(support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    if(device->queueFamilies.graphics != device->queueFamilies.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        uint32_t indices[] = {device->queueFamilies.graphics, device->queueFamilies.present};
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = indices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    free(support.formats);
    free(support.presentModes);

    result = vkCreateSwapchainKHR(device->device, &createInfo, NULL, &swapchain->swapchain);
    ASSERT_ERR(result, {}, "Failed to create swapchain.\n");

    result = acquireSwapChainImages(device, swapchain->swapchain, &swapchain->imageCount, &swapchain->images);
    ASSERT_ERR(result, {}, "Failed to acquire swapchain images.\n");

    swapchain->extent = extent;
    swapchain->format = surfaceFormat.format;
    swapchain->presentMode = presentMode;
    swapchain->framebuffers = NULL;

    result = createImageViews(device, swapchain);
    ASSERT_ERR(result, { free(swapchain->images); }, "Failed to create image views.\n");

    return VK_SUCCESS;
}

void destroySwapChain(Device *device, Swapchain *swapchain) {
    if(swapchain->framebuffers != NULL) {
        for(uint32_t i = 0; i < swapchain->imageCount; i++) {
            vkDestroyFramebuffer(device->device, swapchain->framebuffers[i], NULL);
        }
        free(swapchain->framebuffers);
    }

    for(uint32_t i = 0; i < swapchain->imageCount; i++) {
        vkDestroyImageView(device->device, swapchain->imageViews[i], NULL);
    }

    free(swapchain->imageViews);
    free(swapchain->images);
    vkDestroySwapchainKHR(device->device, swapchain->swapchain, NULL);
}

VkResult setupFramebuffers(Device *device, Swapchain *swapchain, VkRenderPass renderPass) {
    uint32_t result;

    swapchain->framebuffers = (VkFramebuffer*)calloc(swapchain->imageCount, sizeof(VkFramebuffer));

    for(uint32_t i = 0; i < swapchain->imageCount; i++) {
        VkImageView view = swapchain->imageViews[i];
        VkImageView attachments[] = {view};

        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .pAttachments = attachments,
            .attachmentCount = 1,
            .width = swapchain->extent.width,
            .height = swapchain->extent.height,
            .layers = 1,
        };

        result = vkCreateFramebuffer(device->device, &createInfo, NULL, &swapchain->framebuffers[i]);
        if(result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer: %s.\n", string_VkResult(result));
            free(swapchain->framebuffers);
            return result;
        }
    }

    return VK_SUCCESS;
}

VkResult acquireNextImage(Device *device, Swapchain *swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *image) {
    return vkAcquireNextImageKHR(device->device, swapchain->swapchain, timeout, semaphore, fence, image);
}

VkResult acquireSwapChainImages(Device *device, VkSwapchainKHR swapchain, uint32_t *imageCount, VkImage **images) {
    VkResult result;
    result = vkGetSwapchainImagesKHR(device->device, swapchain, imageCount, NULL);
    ASSERT_ERR(result, {}, "Failed to get swapchain image count.\n");

    *images = (VkImage*)calloc(*imageCount, sizeof(VkImage));

    result = vkGetSwapchainImagesKHR(device->device, swapchain, imageCount, *images);
    ASSERT_ERR(result, { free(*images); }, "Failed to get swapchain images.\n");

    return VK_SUCCESS;
}

VkSurfaceFormatKHR chooseSurfaceFormat(VkSurfaceFormatKHR *formats, size_t formatCount) {
    for(size_t i = 0; i < formatCount; i++) {
        if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return formats[i];
        }
    }

    return formats[0];
}

VkPresentModeKHR choosePresentMode(VkPresentModeKHR *presentModes, size_t presentModeCount) {
    for(size_t i = 0; i < presentModeCount; i++) {
        if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR *caps, Window *window) {
    if(caps->currentExtent.width != UINT32_MAX) {
        return caps->currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window->window, &width, &height);

        VkExtent2D extent = {
            (uint32_t)width,
            (uint32_t)height
        };

        extent.width = clamp(extent.width, caps->minImageExtent.width, caps->maxImageExtent.width);
        extent.height = clamp(extent.height, caps->minImageExtent.height, caps->maxImageExtent.height);

        return extent;
    }
}

uint32_t clamp(uint32_t v, uint32_t a, uint32_t b) {
    return v < a ? a : (v > b ? b : v);
}

VkResult createImageViews(
    Device *device,
    Swapchain *swapchain
) {
    uint32_t result;
    swapchain->imageViews = (VkImageView*)calloc(swapchain->imageCount, sizeof(VkImageView));

    for(uint32_t i = 0; i < swapchain->imageCount; i++) {
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain->format,
            .components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
                .levelCount = 1
            },
        };

        result = vkCreateImageView(device->device, &createInfo, NULL, &swapchain->imageViews[i]);
        ASSERT_ERR(result, { free(swapchain->imageViews); }, "Failed to create image views.\n");
    }

    return VK_SUCCESS;
}
