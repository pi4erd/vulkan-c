#include "device_api.h"
#include "device_utils.h"

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

VkResult pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceFeatures *features, StringArray extensions, VkPhysicalDevice *physicalDevice);
VkBool32 getQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices *queueFamilies);

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


VkResult createShaderModule(Device *device, const uint32_t *code, size_t codeSize, VkShaderModule *module) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pCode = code,
        .codeSize = codeSize,
    };

    return vkCreateShaderModule(device->device, &createInfo, NULL, module);
}

void destroyShaderModule(Device *device, VkShaderModule module) {
    vkDestroyShaderModule(device->device, module, NULL);
}

VkResult createPipelineLayout(Device *device, VkPipelineLayoutCreateInfo *info, VkPipelineLayout *layout) {
    return vkCreatePipelineLayout(device->device, info, NULL, layout);
}

void destroyPipelineLayout(Device *device, VkPipelineLayout layout) {
    vkDestroyPipelineLayout(device->device, layout, NULL);
}

VkResult createRenderPass(Device *device, VkRenderPassCreateInfo *info, VkRenderPass *renderPass) {
    return vkCreateRenderPass(device->device, info, NULL, renderPass);
}

void destroyRenderPass(Device *device, VkRenderPass renderPass) {
    vkDestroyRenderPass(device->device, renderPass, NULL);
}

VkResult createGraphicsPipeline(Device *device, VkGraphicsPipelineCreateInfo *info, VkPipeline *pipeline) {
    return vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, info, NULL, pipeline);
}

void destroyPipeline(Device *device, VkPipeline pipeline) {
    vkDestroyPipeline(device->device, pipeline, NULL);
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
