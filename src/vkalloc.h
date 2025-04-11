#ifndef VKALLOC_H_
#define VKALLOC_H_

#include "arrays.h"
#include "device_api.h"
#include <vulkan/vulkan.h>

// Simple arena allocator: does not deallocate until destroyed (for now)
typedef struct {
    Device *device;
    DeviceMemoryArray array;
} VkAlloc;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize memorySize;
} Buffer;

typedef struct {
    Buffer buffer;
    VkAccelerationStructureKHR structure;
} AccelerationStructure;

VkAlloc *createAllocator(Device *device);
void destroyAllocator(VkAlloc *alloc);

VkDeviceAddress getBufferAddress(Device *device, Buffer *buffer);
VkResult createBlas(
    VkAlloc *alloc,
    VkDeviceSize size,
    AccelerationStructure *structure
);
void destroyAccelerationStructure(VkAlloc *alloc, AccelerationStructure *structure);

VkResult allocateDeviceMemory(VkAlloc *alloc, VkMemoryRequirements reqs, VkMemoryPropertyFlags flags, VkDeviceMemory *memory);
VkResult createAllocateBuffer(VkAlloc *alloc, VkBufferCreateInfo *bufferInfo, VkMemoryPropertyFlags flags, Buffer *buffer);
void destroyDeallocateBuffer(VkAlloc *alloc, Buffer *buffer);
void *mapBufferMemory(VkAlloc *alloc, Buffer *buffer);
void unmapBufferMemory(VkAlloc *alloc, Buffer *buffer);

#endif
