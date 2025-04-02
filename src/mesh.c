#include "mesh.h"

#include <assert.h>
#include <vulkan/vulkan_core.h>

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
