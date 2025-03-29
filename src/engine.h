#ifndef ENGINE_H_
#define ENGINE_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include "array.h"

#define MAKE_VERSION(major, minor, patch) VK_VERSION_MAJOR(major) | VK_VERSION_MINOR(minor) | VK_VERSION_PATCH(patch)
#define VK_INSTANCE_FUNC(F, instance) ((PFN_ ## F)vkGetInstanceProcAddr(instance, #F))

typedef struct {
    GLFWwindow *window;
} Window;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
} Device;

VkBool32 debugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data,
    void *userData
);

VkResult createInstance(StringArray extensions, StringArray layers, VkInstance *instance);
VkResult createSurface(Window *window, VkInstance instance, VkSurfaceKHR *surface);
VkResult createDebugMessenger(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback, VkDebugUtilsMessengerEXT *debugMessenger);
VkResult createDevice(VkInstance instance, VkPhysicalDeviceFeatures features, StringArray extensions, Device *device);
void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);
void destroySurface(VkInstance instance, VkSurfaceKHR surface);
void destroyInstance(VkInstance instance);
void destroyDevice(Device *device);

Window createWindow(void);
int windowShouldClose(Window *window);
void pollEvents(void);
void destroyWindow(Window *window);

#endif
