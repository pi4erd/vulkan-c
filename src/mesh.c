#include "mesh.h"
#include "vkalloc.h"

#include <assert.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

Mesh createMesh(
    VkAlloc *allocator,
    const Vertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
) {
    Mesh mesh;
    VkBufferCreateInfo vertexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pQueueFamilyIndices = &allocator->device->queueFamilies.graphics,
        .queueFamilyIndexCount = 1,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .size = sizeof(Vertex) * vertexCount,
    };
    VkBufferCreateInfo indexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pQueueFamilyIndices = &allocator->device->queueFamilies.graphics,
        .queueFamilyIndexCount = 1,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .size = sizeof(uint32_t) * indexCount,
    };
    assert(createAllocateBuffer(
        allocator,
        &vertexBufferInfo,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &mesh.vertexBuffer
    ) == VK_SUCCESS);
    assert(createAllocateBuffer(
        allocator,
        &indexBufferInfo,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &mesh.indexBuffer
    ) == VK_SUCCESS);

    void *ptr;
    ptr = mapBufferMemory(allocator, &mesh.vertexBuffer);
    assert(ptr != NULL);
        memcpy(ptr, vertices, sizeof(Vertex) * vertexCount);
    unmapBufferMemory(allocator, &mesh.vertexBuffer);

    ptr = mapBufferMemory(allocator, &mesh.indexBuffer);
    assert(ptr != NULL);
        memcpy(ptr, indices, sizeof(uint32_t) * indexCount);
    unmapBufferMemory(allocator, &mesh.indexBuffer);

    mesh.indexCount = indexCount;

    return mesh;
}

void destroyMesh(VkAlloc *allocator, Mesh *mesh) {
    destroyDeallocateBuffer(allocator, &mesh->vertexBuffer);
    destroyDeallocateBuffer(allocator, &mesh->indexBuffer);
}

VertexInputDescription vertexDescription(void) {
    VertexInputDescription desc = {
        .attributes = {
            (VkVertexInputAttributeDescription){
                .binding = 0,
                .location = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position),
            },
            (VkVertexInputAttributeDescription){
                .binding = 0,
                .location = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color),
            },
        },
        .bindings = {
            (VkVertexInputBindingDescription){
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
        }
    };

    return desc;
}
