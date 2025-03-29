#include "engine.h"
#include "array.h"

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
VkResult pickPhysicalDevice(VkInstance instance, VkPhysicalDeviceFeatures *features, StringArray extensions, VkPhysicalDevice *physicalDevice);

VkBool32 debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void *userData
) {
    (void)userData;

    fprintf(stderr, "Severity: %s Type: %s. Message:\n%s\n",
        string_VkDebugUtilsMessageSeverityFlagBitsEXT(severity),
        string_VkDebugUtilsMessageTypeFlagBitsEXT(type),
        data->pMessage
    );

    return VK_FALSE;
}

VkResult createInstance(StringArray extensions, StringArray layers, VkInstance *instance) {
    if(!checkInstanceLayers(layers) || !checkInstanceExtensions(extensions)) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vulkan app",
        .applicationVersion = MAKE_VERSION(0, 1, 0),
        .apiVersion = MAKE_VERSION(1, 3, 0),
        .engineVersion = MAKE_VERSION(0, 1, 0),
    };

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .ppEnabledExtensionNames = extensions.elements,
        .enabledExtensionCount = extensions.elementCount,
        .ppEnabledLayerNames = layers.elements,
        .enabledLayerCount = layers.elementCount,
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

VkResult createDevice(VkInstance instance, VkPhysicalDeviceFeatures features, StringArray extensions, Device *device) {
    pickPhysicalDevice(instance, &features, extensions, &device->physicalDevice);
    
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pQueuePriorities = (const float[]){0.0},
        .queueFamilyIndex = 0,
        .queueCount = 1,
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledExtensionNames = extensions.elements,
        .enabledExtensionCount = extensions.elementCount,
        .enabledLayerCount = 0,
        .pQueueCreateInfos = &queueCreateInfo,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &features
    };

    return vkCreateDevice(device->physicalDevice, &createInfo, NULL, &device->device);
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

void destroyDevice(Device *device) {
    vkDestroyDevice(device->device, NULL);
}

Window createWindow(void) {
    Window window;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window.window = glfwCreateWindow(1280, 720, "My app", NULL, NULL);

    return window;
}

int windowShouldClose(Window *window) {
    return glfwWindowShouldClose(window->window);
}

void pollEvents(void) {
    glfwPollEvents();
}

void destroyWindow(Window *window) {
    glfwDestroyWindow(window->window);
    glfwTerminate();
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

VkResult pickPhysicalDevice(VkInstance instance, VkPhysicalDeviceFeatures *requiredFeatures, StringArray requiredExtensions, VkPhysicalDevice *physicalDevice) {
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

        VkBool32 deviceSuitable = VK_TRUE;

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
