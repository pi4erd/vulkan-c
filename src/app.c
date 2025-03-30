#include "app.h"
#include "engine.h"

#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include <stdlib.h>

VulkanState initVulkanState(Window *window, VkBool32 debugging) {
    StringArray extensions = createStringArray(1000);
    StringArray layers = createStringArray(1000);

    VulkanState state = {
        .debugMessenger = VK_NULL_HANDLE,
        .instance = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE,
    };

    uint32_t glfwRequiredExtensionCount;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    
    appendStringArray(&extensions, glfwRequiredExtensions, glfwRequiredExtensionCount);
    addStringToArray(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    if(debugging) {
        addStringToArray(&layers, "VK_LAYER_KHRONOS_validation");
        addStringToArray(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkResult result;
    result = createInstance(extensions, layers, &state.instance);

    destroyStringArray(&extensions);
    destroyStringArray(&layers);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %s.\n", string_VkResult(result));
        exit(1);
    }

    if(debugging) {
        result = createDebugMessenger(state.instance, debugMessengerCallback, &state.debugMessenger);
        if(result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create debug messenger: %s.\n",
                string_VkResult(result));
            exit(1);
        }
    }

    result = createSurface(window, state.instance, &state.surface);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create surface: %s.\n", string_VkResult(result));
        exit(1);
    }

    StringArray deviceExtensions = createStringArray(1000);
    StringArray deviceLayers = createStringArray(1000);

    addStringToArray(&deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if(debugging) {
        addStringToArray(&deviceLayers, "VK_LAYER_KHRONOS_validation");
    }

    VkPhysicalDeviceFeatures features = {0};

    result = createDevice(
        state.instance,
        state.surface,
        &features,
        deviceLayers,
        deviceExtensions,
        &state.device
    );
    destroyStringArray(&deviceExtensions);
    destroyStringArray(&deviceLayers);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device: %s.\n", string_VkResult(result));
        exit(1);
    }

    retrieveQueue(&state.device, state.device.queueFamilies.graphics, &state.graphicsQueue);
    retrieveQueue(&state.device, state.device.queueFamilies.present, &state.presentQueue);

    result = createSwapChain(&state.device, window, state.surface, &state.swapchain);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain: %s.\n", string_VkResult(result));
        exit(1);
    }

    return state;
}

void destroyVulkanState(VulkanState *vulkanState) {
    destroySwapChain(&vulkanState->device, &vulkanState->swapchain);
    destroyDevice(&vulkanState->device);

    if(vulkanState->debugMessenger != VK_NULL_HANDLE) {
        destroyDebugMessenger(vulkanState->instance, vulkanState->debugMessenger);
    }

    destroySurface(vulkanState->instance, vulkanState->surface);
    destroyInstance(vulkanState->instance);
}