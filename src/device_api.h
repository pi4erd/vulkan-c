#ifndef DEVICE_API_H_
#define DEVICE_API_H_

#include "array.h"
#include "window.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    uint32_t graphics;
    uint32_t present;
} QueueFamilyIndices;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    QueueFamilyIndices queueFamilies;
} Device;

VkResult createDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    VkPhysicalDeviceFeatures *features,
    StringArray layers,
    StringArray extensions,
    Device *device
);
void destroyDevice(Device *device);

void retrieveQueue(Device *device, uint32_t familyIndex, VkQueue *queue);

#pragma region API

#pragma endregion

#endif
