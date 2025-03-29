#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include "array.h"
#include "engine.h"

typedef struct {
    VkInstance instance;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debugMessenger;
} VulkanState;

VulkanState initVulkanState(Window *window, VkBool32 debugging);
void destroyVulkanState(VulkanState *vulkanState);

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

    VkResult result = createInstance(extensions, layers, &state.instance);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %s.\n", string_VkResult(result));
        exit(1);
    }

    destroyStringArray(&extensions);
    destroyStringArray(&layers);

    if(debugging) {
        VkResult debugResult = createDebugMessenger(state.instance, debugMessengerCallback, &state.debugMessenger);
        if(debugResult != VK_SUCCESS) {
            fprintf(stderr, "Failed to create debug messenger: %s.\n",
                string_VkResult(debugResult));
        }
    }

    VkResult surfaceResult = createSurface(window, state.instance, &state.surface);
    if(surfaceResult != VK_SUCCESS) {
        fprintf(stderr, "Failed to create surface: %s.\n", string_VkResult(surfaceResult));
    }

    return state;
}

void destroyVulkanState(VulkanState *vulkanState) {
    if(vulkanState->debugMessenger != VK_NULL_HANDLE) {
        destroyDebugMessenger(vulkanState->instance, vulkanState->debugMessenger);
    }

    destroySurface(vulkanState->instance, vulkanState->surface);
    
    destroyInstance(vulkanState->instance);
}

int main(void) {
    Window window = createWindow();
    VulkanState state = initVulkanState(&window, VK_TRUE);

    while(!windowShouldClose(&window)) {
        pollEvents();
    }

    destroyVulkanState(&state);
    destroyWindow(&window);

    return 0;
}


