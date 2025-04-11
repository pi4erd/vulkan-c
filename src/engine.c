#include "engine.h"
#include "array.h"
#include "device_api.h"

#include <GLFW/glfw3.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

/// Checks if instance extensions are available
VkBool32 checkInstanceExtensions(StringArray extensions);
VkBool32 checkInstanceLayers(StringArray layers);

PFN_vkVoidFunction getInstanceProcAddrChecked(VkInstance instance, const char *name) {
    PFN_vkVoidFunction func = vkGetInstanceProcAddr(instance, name);

    if(!func) {
        fprintf(stderr, "Failed to find instance procedure: %s. May be unavailable in driver.\n", name);
        exit(1);
    }

    return func;
}

PFN_vkVoidFunction getDeviceProcAddrChecked(VkDevice device, const char *name) {
    PFN_vkVoidFunction func = vkGetDeviceProcAddr(device, name);

    if(!func) {
        fprintf(stderr, "Failed to find device procedure: %s. May be unavailable in driver.\n", name);
        exit(1);
    }

    return func;
}

VkBool32 debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void *userData
) {
    (void)userData;

    const char *severityStr, *typeStr;

    switch(severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severityStr = " ERROR ";
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severityStr = "WARNING";
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severityStr = " INFO  ";
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severityStr = "VERBOSE";
        break;
        default:
        severityStr = "UNKNOWN";
        break;
    }

    switch(type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        typeStr = "  GENERAL  ";
        break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        typeStr = "PERFORMANCE";
        break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        typeStr = "VALIDATION ";
        break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
        typeStr = "    DAB    ";
        break;
        default:
        typeStr = "  UNKNOWN  ";
        break;
    }

    fprintf(stderr, "%s (%s): %s\n",
        severityStr,
        typeStr,
        data->pMessage
    );

    return VK_FALSE;
}

VkResult createInstance(StringArray extensions, StringArray layers, VkBool32 portability, VkInstance *instance) {
    if(portability) {
        StringArrayAddElement(&extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }

    if(!checkInstanceLayers(layers)) {
        return VK_ERROR_LAYER_NOT_PRESENT;
    } else if(!checkInstanceExtensions(extensions)) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vulkan app",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
    };

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .ppEnabledExtensionNames = extensions.elements,
        .enabledExtensionCount = extensions.elementCount,
        .ppEnabledLayerNames = layers.elements,
        .enabledLayerCount = layers.elementCount,
        .flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
    };

    return vkCreateInstance(&instanceInfo, NULL, instance);
}

VkResult createSurface(Window *window, VkInstance instance, VkSurfaceKHR *surface) {
    return glfwCreateWindowSurface(instance, window->window, NULL, surface);
}

VkResult createDebugMessenger(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback, VkDebugUtilsMessengerEXT *debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .pfnUserCallback = callback,
        .pUserData = NULL,
        .pNext = NULL,
    };

    return VK_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT, instance)(instance, &createInfo, NULL, debugMessenger);
}

void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
    VK_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT, instance)(instance, messenger, NULL);
}

void destroySurface(VkInstance instance, VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(instance, surface, NULL);
}

void destroyInstance(VkInstance instance) {
    vkDestroyInstance(instance, NULL);
}

void transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage
) {
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1,
    };

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
    };

    cmdPipelineBarrier(
        commandBuffer,
        srcStage,
        dstStage,
        0, 0, NULL, 0, NULL,
        1, &barrier
    );
}

VkBool32 checkInstanceExtensions(StringArray extensions) {
    uint32_t extensionCount;
    uint32_t result;

    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    if(result != VK_SUCCESS) return result;

    VkExtensionProperties *properties = (VkExtensionProperties*)calloc(extensionCount, sizeof(VkExtensionProperties));

    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, properties);
    if(result != VK_SUCCESS) {
        free(properties);
        return result;
    }

    VkBool32 allFound = VK_TRUE;
    for(size_t i = 0; i < extensions.elementCount; i++) {
        VkBool32 found = VK_FALSE;
        for(uint32_t j = 0; j < extensionCount; j++) {
            if(strcmp(extensions.elements[i], properties[j].extensionName) == 0) {
                found = VK_TRUE;
                break;
            }
        }

        if(!found) {
            fprintf(stderr, "No required extension '%s' found.\n", extensions.elements[i]);
            allFound = VK_FALSE;
        }
    }

    free(properties);

    return allFound;
}

VkBool32 checkInstanceLayers(StringArray layers) {
    uint32_t layerCount;
    uint32_t result;

    result = vkEnumerateInstanceExtensionProperties(NULL, &layerCount, NULL);
    if(result != VK_SUCCESS) return result;

    VkLayerProperties *properties = (VkLayerProperties*)calloc(layerCount, sizeof(VkLayerProperties));

    result = vkEnumerateInstanceLayerProperties(&layerCount, properties);
    if(result != VK_SUCCESS) {
        free(properties);
        return result;
    }

    VkBool32 allFound = VK_TRUE;
    for(size_t i = 0; i < layers.elementCount; i++) {
        VkBool32 found = VK_FALSE;
        for(uint32_t j = 0; j < layerCount; j++) {
            if(strcmp(layers.elements[i], properties[j].layerName) == 0) {
                found = VK_TRUE;
                break;
            }
        }

        if(!found) {
            fprintf(stderr, "No required layer '%s' found.\n", layers.elements[i]);
            allFound = VK_FALSE;
        }
    }

    free(properties);

    return allFound;
}
