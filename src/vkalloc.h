#ifndef VKALLOC_H_
#define VKALLOC_H_

#include "array.h"
#include "device_api.h"
#include <vulkan/vulkan.h>

DEFINE_ARRAY(DeviceMemory, VkDeviceMemory)

// Simple arena allocator: does not deallocate until destroyed
typedef struct {
    Device *device;
    DeviceMemoryArray array;
} VkAlloc;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize memorySize;
} Buffer;

VkAlloc *createAllocator(Device *device);
void destroyAllocator(VkAlloc *alloc);

VkResult allocateDeviceMemory(VkAlloc *alloc, VkMemoryRequirements reqs, VkMemoryPropertyFlags flags, VkDeviceMemory *memory);
VkResult createAllocateBuffer(VkAlloc *alloc, VkBufferCreateInfo *bufferInfo, VkMemoryPropertyFlags flags, Buffer *buffer);
void destroyDeallocateBuffer(VkAlloc *alloc, Buffer *buffer);
void *mapBufferMemory(VkAlloc *alloc, Buffer *buffer);
void unmapBufferMemory(VkAlloc *alloc, Buffer *buffer);

#endif
