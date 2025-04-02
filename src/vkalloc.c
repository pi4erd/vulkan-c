#include "vkalloc.h"
#include "device_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDeviceMemoryProperties props, VkMemoryPropertyFlags flags);

VkAlloc *createAllocator(Device *device) {
    VkAlloc *allocator = (VkAlloc*)calloc(1, sizeof(VkAlloc));
    allocator->array = createDeviceMemoryArray(1000);
    allocator->device = device;

    return allocator;
}

void destroyAllocator(VkAlloc *alloc) {
    for(size_t i = 0; i < alloc->array.elementCount; i++) {
        vkFreeMemory(alloc->device->device, alloc->array.elements[i], NULL);
    }

    destroyDeviceMemoryArray(&alloc->array);
    free(alloc);
}

VkResult allocateDeviceMemory(VkAlloc *alloc, VkMemoryRequirements reqs, VkMemoryPropertyFlags flags, VkDeviceMemory *memory) {
    uint32_t result;

    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(alloc->device->physicalDevice, &props);

    uint32_t memoryType = findMemoryType(
        reqs.memoryTypeBits,
        props,
        flags
    );

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = reqs.size,
        .memoryTypeIndex = memoryType,
    };

    VkDeviceMemory mem;
    result = vkAllocateMemory(alloc->device->device, &allocInfo, NULL, &mem);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate device memory: %s.\n", string_VkResult(result));
        return result;
    }

    DeviceMemoryArrayAddElement(&alloc->array, mem);
    *memory = mem;

    return VK_SUCCESS;
}

VkResult createAllocateBuffer(VkAlloc *alloc, VkBufferCreateInfo *bufferInfo, VkMemoryPropertyFlags flags, Buffer *buffer) {
    VkBuffer buf;
    VkResult result;
    result = createBuffer(alloc->device, bufferInfo, &buf);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer: %s.\n", string_VkResult(result));
        return result;
    }

    VkMemoryRequirements reqs = getBufferMemoryRequirements(alloc->device, buf);
    VkDeviceMemory memory;

    result = allocateDeviceMemory(alloc, reqs, flags, &memory);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate device memory: %s.\n", string_VkResult(result));
        return result;
    }

    result = bindBufferMemory(alloc->device, buf, memory, 0);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to bind device memory: %s.\n", string_VkResult(result));
        return result;
    }

    *buffer = (Buffer){
        .buffer = buf,
        .memory = memory,
        .memorySize = bufferInfo->size,
    };

    return VK_SUCCESS;
}

void destroyDeallocateBuffer(VkAlloc *alloc, Buffer *buffer) {
    destroyBuffer(alloc->device, buffer->buffer);
}

void *mapBufferMemory(VkAlloc *alloc, Buffer *buffer) {
    void *data;
    uint32_t result;

    result = mapMemory(alloc->device, buffer->memory, 0, buffer->memorySize, &data);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to map memory: %s.\n", string_VkResult(result));
        return NULL;
    }

    return data;
}

void unmapBufferMemory(VkAlloc *alloc, Buffer *buffer) {
    unmapMemory(alloc->device, buffer->memory);
}

uint32_t findMemoryType(uint32_t typeFilter, VkPhysicalDeviceMemoryProperties props, VkMemoryPropertyFlags flags) {
    for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    exit(69);
}
