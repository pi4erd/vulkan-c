#include "device_api.h"
#include "window.h"

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#define ASSERT_ERR(R, BLOCK, ...) if(R != VK_SUCCESS) { \
        BLOCK \
        fprintf(stderr, __VA_ARGS__); \
        return R; \
    }

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    size_t formatCount;
    VkPresentModeKHR *presentModes;
    size_t presentModeCount;
} SwapChainSupport;

VkResult pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceFeatures *features, StringArray extensions, VkPhysicalDevice *physicalDevice);
VkBool32 getQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices *queueFamilies);
VkResult querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, SwapChainSupport *support);
VkSurfaceFormatKHR chooseSurfaceFormat(VkSurfaceFormatKHR *formats, size_t formatCount);
VkPresentModeKHR choosePresentMode(VkPresentModeKHR *presentModes, size_t presentModeCount);
VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR *caps, Window *window);
uint32_t clamp(uint32_t, uint32_t, uint32_t);

void retrieveQueue(Device *device, uint32_t familyIndex, VkQueue *queue) {
    vkGetDeviceQueue(device->device, familyIndex, 0, queue);
}

VkResult createDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceFeatures *features, StringArray layers, StringArray extensions, Device *device) {
    uint32_t result;
    result = pickPhysicalDevice(instance, surface, features, extensions, &device->physicalDevice);
    
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
        .pEnabledFeatures = features
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

VkResult createSwapChain(Device *device, Window *window, VkSurfaceKHR surface, VkSwapchainKHR *swapchain) {
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

    return vkCreateSwapchainKHR(device->device, &createInfo, NULL, swapchain);
}

void destroySwapChain(Device *device, VkSwapchainKHR swapchain) {
    vkDestroySwapchainKHR(device->device, swapchain, NULL);
}

VkResult acquireSwapChainImages(Device *device, VkSwapchainKHR swapchain, uint32_t *imageCount, VkImage **images) {
    VkResult result;
    result = vkGetSwapchainImagesKHR(device->device, swapchain, imageCount, NULL);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get swapchain image count.\n");
        return result;
    }
    *images = (VkImage*)calloc(*imageCount, sizeof(VkImage));
    result = vkGetSwapchainImagesKHR(device->device, swapchain, imageCount, *images);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acquire swapchain images.\n");
        return result;
    }

    return VK_SUCCESS;
}

VkBool32 getQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices *queueFamilies) {
    VkBool32 graphicsFound = VK_FALSE, presentFound = VK_FALSE;
    uint32_t graphics, present;

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


VkResult pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceFeatures *requiredFeatures, StringArray requiredExtensions, VkPhysicalDevice *physicalDevice) {
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

        // TODO: Check features present
        (void)requiredFeatures;

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
