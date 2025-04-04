#include "app.h"
#include "array.h"
#include "device_api.h"
#include "engine.h"
#include "mesh.h"
#include "swapchain.h"
#include "vkalloc.h"
#include "window.h"

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES_IN_FLIGHT 2

typedef struct VKSTATE {
    VkInstance instance;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debugMessenger;
    Device device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    Swapchain swapchain;
    VkPipelineLayout layout;
    VkRenderPass renderPass;
    VkPipeline graphicsPipeline;
    
    VkCommandPool commandPool;

    VkCommandBuffer *commandBuffers;
    VkSemaphore *imageAvailableSemas, *renderFinishedSemas;
    VkFence *inFlightFences;

    VkAlloc *allocator;
    Mesh mesh;

    VkBool32 portability;
    VkBool32 framebufferResized;
    uint32_t currentFrame;
} VulkanState;

void CreateGraphicsPipeline(VulkanState *state);
void CopyBuffer(VulkanState *state, Buffer *dst, Buffer *src);
Mesh CreateMesh(
    VulkanState *state,
    const Vertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
);
void DestroyMesh(VulkanState *state, Mesh *mesh);
Buffer CreateBufferGQueue(VulkanState *state, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags);

VulkanState *initVulkanState(Window *window, VkBool32 debugging) {
    StringArray extensions = createStringArray(1000);
    StringArray layers = createStringArray(1000);

    VulkanState *state = calloc(1, sizeof(VulkanState));

    uint32_t glfwRequiredExtensionCount;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    
    StringArrayAppendConstArray(&extensions, glfwRequiredExtensions, glfwRequiredExtensionCount);
    StringArrayAddElement(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    if(debugging) {
        StringArrayAddElement(&layers, "VK_LAYER_KHRONOS_validation");
        StringArrayAddElement(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkResult result;
    VkBool32 portability;

#ifdef __MACH__
    portability = VK_TRUE;
    addStringToArray(&extensions, "VK_EXT_metal_surface");
#else
    portability = VK_FALSE;
#endif

    state->portability = portability;

    result = createInstance(extensions, layers, portability, &state->instance);

    destroyStringArray(&extensions);
    destroyStringArray(&layers);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %s.\n", string_VkResult(result));
        exit(1);
    }

    if(debugging) {
        result = createDebugMessenger(state->instance, debugMessengerCallback, &state->debugMessenger);
        if(result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create debug messenger: %s.\n",
                string_VkResult(result));
            exit(1);
        }
    }

    result = createSurface(window, state->instance, &state->surface);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create surface: %s.\n", string_VkResult(result));
        exit(1);
    }

    StringArray deviceExtensions = createStringArray(1000);
    StringArray deviceLayers = createStringArray(1000);

    if(portability) {
        StringArrayAddElement(&deviceExtensions, "VK_KHR_portability_subset");
    }
    StringArrayAddElement(&deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if(debugging) {
        StringArrayAddElement(&deviceLayers, "VK_LAYER_KHRONOS_validation");
    }

    VkPhysicalDeviceFeatures features = {0};

    result = createDevice(
        state->instance,
        state->surface,
        &features,
        deviceLayers,
        deviceExtensions,
        &state->device
    );
    destroyStringArray(&deviceExtensions);
    destroyStringArray(&deviceLayers);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device: %s.\n", string_VkResult(result));
        exit(1);
    }

    retrieveQueue(&state->device, state->device.queueFamilies.graphics, &state->graphicsQueue);
    retrieveQueue(&state->device, state->device.queueFamilies.present, &state->presentQueue);

    result = createSwapChain(&state->device, window, state->surface, &state->swapchain);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain: %s.\n", string_VkResult(result));
        exit(1);
    }

    CreateGraphicsPipeline(state);

    result = setupFramebuffers(&state->device, &state->swapchain, state->renderPass);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to setup framebuffers: %s.\n", string_VkResult(result));
        exit(1);
    }

    result = createCommandPool(
        &state->device,
        state->device.queueFamilies.graphics,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        &state->commandPool
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool: %s.\n", string_VkResult(result));
        exit(1);
    }

    result = allocateCommandBuffers(
        &state->device,
        state->commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        FRAMES_IN_FLIGHT,
        &state->commandBuffers
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffer: %s.\n", string_VkResult(result));
        exit(1);
    }

    state->imageAvailableSemas = (VkSemaphore*)calloc(FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    state->renderFinishedSemas = (VkSemaphore*)calloc(FRAMES_IN_FLIGHT, sizeof(VkSemaphore));
    state->inFlightFences = (VkFence*)calloc(FRAMES_IN_FLIGHT, sizeof(VkFence));
    state->framebufferResized = VK_FALSE;
    state->currentFrame = 0;

    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        assert(createSemaphore(&state->device, &state->imageAvailableSemas[i]) == VK_SUCCESS);
        assert(createSemaphore(&state->device, &state->renderFinishedSemas[i]) == VK_SUCCESS);
        assert(createFence(&state->device, VK_TRUE, &state->inFlightFences[i]) == VK_SUCCESS);
    }

    state->allocator = createAllocator(&state->device);

    // Game logic starts here :)
    const Vertex vertices[] = {
        {{-0.8f, -0.8f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.8f, -0.8f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.8f, 0.8f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.8f, 0.8f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };
    const uint32_t indices[] = {
        0, 2, 1,
        0, 3, 2,
    };

    state->mesh = CreateMesh(
        state,
        vertices, sizeof(vertices) / sizeof(Vertex),
        indices, sizeof(indices) / sizeof(uint32_t)
    );

    return state;
}

void destroyVulkanState(VulkanState *vulkanState) {
    assert(waitIdle(&vulkanState->device) == VK_SUCCESS);

    DestroyMesh(vulkanState, &vulkanState->mesh);
    destroyAllocator(vulkanState->allocator);

    for(size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        destroySemaphore(&vulkanState->device, vulkanState->imageAvailableSemas[i]);
        destroySemaphore(&vulkanState->device, vulkanState->renderFinishedSemas[i]);
        destroyFence(&vulkanState->device, vulkanState->inFlightFences[i]);
    } 

    free(vulkanState->imageAvailableSemas);
    free(vulkanState->renderFinishedSemas);
    free(vulkanState->inFlightFences);

    destroyCommandPool(&vulkanState->device, vulkanState->commandPool);
    free(vulkanState->commandBuffers);

    destroyPipeline(&vulkanState->device, vulkanState->graphicsPipeline);
    destroyPipelineLayout(&vulkanState->device, vulkanState->layout);
    destroyRenderPass(&vulkanState->device, vulkanState->renderPass);

    destroySwapChain(&vulkanState->device, &vulkanState->swapchain);
    destroyDevice(&vulkanState->device);

    if(vulkanState->debugMessenger != VK_NULL_HANDLE) {
        destroyDebugMessenger(vulkanState->instance, vulkanState->debugMessenger);
    }

    destroySurface(vulkanState->instance, vulkanState->surface);
    destroyInstance(vulkanState->instance);

    free(vulkanState);
}

void recordCommandBuffer(VulkanState *vulkanState, uint32_t imageIndex) {
    VkCommandBuffer cmdBuffer = vulkanState->commandBuffers[vulkanState->currentFrame];

    assert(resetCommandBuffer(cmdBuffer) == VK_SUCCESS);
    assert(beginSimpleCommandBuffer(cmdBuffer) == VK_SUCCESS);

    VkClearValue clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vulkanState->renderPass,
        .framebuffer = vulkanState->swapchain.framebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = vulkanState->swapchain.extent,
        },
        .pClearValues = &clearValue,
        .clearValueCount = 1,
    };
    cmdBeginRenderPass(cmdBuffer, &beginInfo);
    {
        cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanState->graphicsPipeline);

        VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = (float)vulkanState->swapchain.extent.width,
            .height = (float)vulkanState->swapchain.extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        cmdSetViewport(cmdBuffer, viewport);

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = vulkanState->swapchain.extent,
        };
        cmdSetScissor(cmdBuffer, scissor);

        VkBuffer vertexBuffers[] = {vulkanState->mesh.vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        cmdBindVertexBuffers(
            cmdBuffer,
            (UInt32Range){0, 1},
            vertexBuffers,
            offsets
        );
        cmdBindIndexBuffer(
            cmdBuffer,
            vulkanState->mesh.indexBuffer.buffer,
            0,
            VK_INDEX_TYPE_UINT32
        );
        cmdDrawIndexed(
            cmdBuffer,
            (UInt32Range){0, vulkanState->mesh.indexCount},
            (UInt32Range){0, 1},
            0
        );
    }
    cmdEndRenderPass(cmdBuffer);

    assert(endCommandBuffer(cmdBuffer) == VK_SUCCESS);
}

VkBool32 getImage(VulkanState *vulkanState, Window *window, uint32_t *image) {
    assert(waitForFence(&vulkanState->device, vulkanState->inFlightFences[vulkanState->currentFrame], UINT64_MAX) == VK_SUCCESS);

    uint32_t imageIndex;
    VkResult getImageResult = acquireNextImage(
        &vulkanState->device,
        &vulkanState->swapchain,
        UINT64_MAX,
        vulkanState->imageAvailableSemas[vulkanState->currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    switch(getImageResult) {
        case VK_ERROR_OUT_OF_DATE_KHR:
        recreateSwapChain(vulkanState, window);
        return VK_FALSE;
        case VK_SUBOPTIMAL_KHR:
        case VK_SUCCESS:
        break;
        default:
        fprintf(stderr, "Failed to get image: %s.\n", string_VkResult(getImageResult));
        glfwSetWindowShouldClose(window->window, GLFW_TRUE);
        return VK_FALSE;
    }

    assert(resetFence(&vulkanState->device, vulkanState->inFlightFences[vulkanState->currentFrame]) == VK_SUCCESS);

    *image = imageIndex;
    return VK_TRUE;
}

void renderAndPresent(VulkanState *vulkanState, Window *window, uint32_t imageIndex) {
    VkSemaphore waitSemaphores[] = {vulkanState->imageAvailableSemas[vulkanState->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {vulkanState->renderFinishedSemas[vulkanState->currentFrame]};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(VkSemaphore),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .pCommandBuffers = &vulkanState->commandBuffers[vulkanState->currentFrame],
        .commandBufferCount = 1,
        .pSignalSemaphores = signalSemaphores,
        .signalSemaphoreCount = 1,
    };

    VkResult result;
    result = queueSubmit(vulkanState->graphicsQueue, 1, &submitInfo, vulkanState->inFlightFences[vulkanState->currentFrame]);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit draw to queue: %s.\n", string_VkResult(result));
        return;
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = sizeof(signalSemaphores) / sizeof(VkSemaphore),
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &vulkanState->swapchain.swapchain,
        .pImageIndices = &imageIndex,
    };

    result = queuePresent(vulkanState->presentQueue, &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkanState->framebufferResized) {
        recreateSwapChain(vulkanState, window);
        vulkanState->framebufferResized = VK_FALSE;
    } else if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to queue present: %s.\n", string_VkResult(result));
        return;
    }

    vulkanState->currentFrame = (vulkanState->currentFrame + 1) % FRAMES_IN_FLIGHT;
}

void recreateSwapChain(VulkanState *vulkanState, Window *window) {
    int width = 0, height = 0;
    getFramebufferSize(window, &width, &height);
    while(width == 0 || height == 0) {
        getFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    waitIdle(&vulkanState->device);
    
    destroySwapChain(&vulkanState->device, &vulkanState->swapchain);
    assert(createSwapChain(
        &vulkanState->device,
        window,
        vulkanState->surface,
        &vulkanState->swapchain
    ) == VK_SUCCESS);

    assert(setupFramebuffers(&vulkanState->device, &vulkanState->swapchain, vulkanState->renderPass) == VK_SUCCESS);
}

void framebufferResized(VulkanState *vulkanState) {
    vulkanState->framebufferResized = VK_TRUE;
}

#include <main.frag.h>
#include <main.vert.h>

void CreateGraphicsPipeline(VulkanState *state) {
    uint32_t result;

    VkShaderModule vertex, fragment;
    result = createShaderModule(&state->device, (const uint32_t*)main_vert_h, sizeof(main_vert_h), &vertex);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader module: %s\n",
            string_VkResult(result));
        exit(1);
    }

    result = createShaderModule(&state->device, (const uint32_t*)main_frag_h, sizeof(main_frag_h), &fragment);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader module: %s\n",
            string_VkResult(result));
        exit(1);
    }

    VkPipelineShaderStageCreateInfo vertexStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragmentStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamicStates,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
    };

    VertexInputDescription desc = vertexDescription();

    VkPipelineVertexInputStateCreateInfo inputStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pVertexBindingDescriptions = desc.bindings,
        .vertexBindingDescriptionCount = sizeof(desc.bindings) / sizeof(VkVertexInputBindingDescription),
        .pVertexAttributeDescriptions = desc.attributes,
        .vertexAttributeDescriptionCount = sizeof(desc.attributes) / sizeof(VkVertexInputAttributeDescription),
    };

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_R_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .pAttachments = &colorBlendAttachment,
        .attachmentCount = 1,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    result = createPipelineLayout(&state->device, &pipelineLayoutInfo, &state->layout);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkAttachmentDescription rpColorAttachment = {
        .format = state->swapchain.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference rpColorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pColorAttachments = &rpColorAttachmentRef,
        .colorAttachmentCount = 1,
    };

    VkSubpassDependency subpassDependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &rpColorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    };

    result = createRenderPass(&state->device, &renderPassInfo, &state->renderPass);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages = shaderStages,
        .stageCount = sizeof(shaderStages) / sizeof(VkPipelineShaderStageCreateInfo),
        .pVertexInputState = &inputStateInfo,
        .pInputAssemblyState = &assemblyInfo,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicStateInfo,
        .layout = state->layout,
        .renderPass = state->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };
    result = createGraphicsPipeline(&state->device, &pipelineInfo, &state->graphicsPipeline);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline: %s.\n", string_VkResult(result));
        exit(1);
    }

    destroyShaderModule(&state->device, fragment);
    destroyShaderModule(&state->device, vertex);
}

void CopyBuffer(VulkanState *state, Buffer *dst, Buffer *src) {
    VkCommandBuffer transferBuffer;
    assert(allocateCommandBuffer(
        &state->device,
        state->commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        &transferBuffer
    ) == VK_SUCCESS);

    assert(beginOneTimeCommandBuffer(transferBuffer) == VK_SUCCESS);

    VkBufferCopy copyRegion = {
        .dstOffset = 0,
        .srcOffset = 0,
        .size = src->memorySize,
    };
    cmdCopyBuffer(transferBuffer, src->buffer, dst->buffer, 1, &copyRegion);

    assert(endCommandBuffer(transferBuffer) == VK_SUCCESS);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = &transferBuffer,
        .commandBufferCount = 1,
    };
    assert(queueSubmit(state->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) == VK_SUCCESS);
    assert(queueWaitIdle(state->graphicsQueue) == VK_SUCCESS);

    freeCommandBuffers(&state->device, state->commandPool, &transferBuffer, 1);
}

Mesh CreateMesh(
    VulkanState *state,
    const Vertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
) {
    Mesh mesh;

    mesh.vertexBuffer = CreateBufferGQueue(
        state,
        sizeof(Vertex) * vertexCount,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    mesh.indexBuffer = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    Buffer vertexStaging = CreateBufferGQueue(
        state,
        sizeof(Vertex) * vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    Buffer indexStaging = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    void *ptr;
    ptr = mapBufferMemory(state->allocator, &vertexStaging);
    assert(ptr != NULL);
        memcpy(ptr, vertices, sizeof(Vertex) * vertexCount);
    unmapBufferMemory(state->allocator, &vertexStaging);

    ptr = mapBufferMemory(state->allocator, &indexStaging);
    assert(ptr != NULL);
        memcpy(ptr, indices, sizeof(uint32_t) * indexCount);
    unmapBufferMemory(state->allocator, &indexStaging);

    CopyBuffer(state, &mesh.vertexBuffer, &vertexStaging);
    CopyBuffer(state, &mesh.indexBuffer, &indexStaging);

    destroyDeallocateBuffer(state->allocator, &vertexStaging);
    destroyDeallocateBuffer(state->allocator, &indexStaging);

    mesh.indexCount = indexCount;

    return mesh;
}

void DestroyMesh(VulkanState *state, Mesh *mesh) {
    destroyDeallocateBuffer(state->allocator, &mesh->vertexBuffer);
    destroyDeallocateBuffer(state->allocator, &mesh->indexBuffer);
}

Buffer CreateBufferGQueue(VulkanState *state, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags) {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pQueueFamilyIndices = &state->device.queueFamilies.graphics,
        .queueFamilyIndexCount = 1,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .size = bufferSize,
    };
    Buffer buffer;
    assert(createAllocateBuffer(state->allocator, &bufferInfo, memoryFlags, &buffer) == VK_SUCCESS);

    return buffer;
}
