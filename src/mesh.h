#ifndef MESH_H_
#define MESH_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "device_api.h"
#include "vkalloc.h"

typedef struct {
    float x, y;
} Vector2f;

typedef struct {
    float x, y, z;
} Vector3f;

typedef struct {
    float x, y, z, w;
} Vector4f;

typedef struct {
    Vector3f position;
    Vector3f color;
} Vertex;

typedef struct {
    VkVertexInputAttributeDescription attributes[2];
    VkVertexInputBindingDescription bindings[1];
} VertexInputDescription;

typedef struct {
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t indexCount;
} Mesh;

Mesh createMesh(
    VkAlloc *allocator,
    const Vertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
);
void destroyMesh(VkAlloc *allocator, Mesh *mesh);

VertexInputDescription vertexDescription(void);

#endif
