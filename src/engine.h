#ifndef ENGINE_H_
#define ENGINE_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "device_api.h"
#include "window.h"
#include "array.h"

#define VK_INSTANCE_FUNC(F, instance) ((PFN_ ## F)getInstanceProcAddrChecked(instance, #F))
#define VK_DEVICE_FUNC(F, device) ((PFN_ ## F)getDeviceProcAddrChecked(device, #F))

PFN_vkVoidFunction getInstanceProcAddrChecked(VkInstance instance, const char *name);
PFN_vkVoidFunction getDeviceProcAddrChecked(VkDevice device, const char *name);

VkBool32 debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void *userData
);

VkResult createInstance(
    StringArray extensions,
    StringArray layers,
    VkBool32 portability,
    VkInstance *instance
);
VkResult createSurface(Window *window, VkInstance instance, VkSurfaceKHR *surface);
VkResult createDebugMessenger(
    VkInstance instance,
    PFN_vkDebugUtilsMessengerCallbackEXT callback,
    VkDebugUtilsMessengerEXT *debugMessenger
);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
void destroySurface(VkInstance instance, VkSurfaceKHR surface);
void destroyInstance(VkInstance instance);

void transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage
);

#endif
