#include "app.h"
#include "arrays.h"
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

#define VK_KHR_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define VK_EXT_METAL_SURFACE_EXTENSION_NAME "VK_EXT_metal_surface"
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"

#define FRAMES_IN_FLIGHT 2

typedef struct VKSTATE {
    VkInstance instance;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debugMessenger;
    Device device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    Swapchain swapchain;
    
    VkCommandPool commandPool;

    VkCommandBuffer *commandBuffers;
    VkSemaphore *imageAvailableSemas, *renderFinishedSemas;
    VkFence *inFlightFences;

    VkAlloc *allocator;

    VkPipelineLayout layout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout rayTracingSetLayout;
    VkPipelineLayout rayTracingLayout;
    VkPipeline rayTracingPipeline;

    Mesh mesh;
    AccelerationStructure blas;

    VkBool32 portability;
    VkBool32 framebufferResized;
    uint32_t currentFrame;
} VulkanState;

void CreateGraphicsPipeline(VulkanState *state);
void CreateRayTracingPipeline(VulkanState *state);
void CopyBuffer(VulkanState *state, Buffer *dst, Buffer *src);
Mesh CreateMesh(
    VulkanState *state,
    const Vertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
);
Mesh CreateRtMesh(
    VulkanState *state,
    const RtVertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
);
void DestroyMesh(VulkanState *state, Mesh *mesh);
Buffer CreateBufferGQueue(
    VulkanState *state,
    VkDeviceSize bufferSize,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    VkMemoryAllocateFlags allocateFlags
);
AccelerationStructure CreateAccelerationStructure(
    VulkanState *state,
    VkBuffer vertexBuffer,
    uint32_t vertexCount,
    VkBuffer indexBuffer,
    uint32_t indexCount
);

VulkanState *initVulkanState(Window *window, VkBool32 debugging) {
    StringArray extensions = StringArrayNew(1000);
    StringArray layers = StringArrayNew(1000);

    VulkanState *state = calloc(1, sizeof(VulkanState));

    uint32_t glfwRequiredExtensionCount;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    
    StringArrayAppendConstArray(&extensions, glfwRequiredExtensions, glfwRequiredExtensionCount);
    StringArrayAddElement(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    if(debugging) {
        StringArrayAddElement(&layers, VK_KHR_VALIDATION_LAYER_NAME);
        StringArrayAddElement(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkResult result;
    VkBool32 portability;

#ifdef __MACH__
    portability = VK_TRUE;
#else
    portability = VK_FALSE;
#endif

    state->portability = portability;

    if(portability) {
        StringArrayAddElement(&extensions, VK_EXT_METAL_SURFACE_EXTENSION_NAME);
    }

    result = createInstance(extensions, layers, portability, &state->instance);

    StringArrayDestroy(&extensions);
    StringArrayDestroy(&layers);

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

    StringArray deviceExtensions = StringArrayNew(1000);
    StringArray deviceLayers = StringArrayNew(1000);

    if(portability) {
        StringArrayAddElement(&deviceExtensions, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
    StringArrayAddElement(&deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    StringArrayAddElement(&deviceExtensions, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    if(debugging) {
        StringArrayAddElement(&deviceLayers, VK_KHR_VALIDATION_LAYER_NAME);
    }

    // Add ray support
    StringArrayAddElement(&deviceExtensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    StringArrayAddElement(&deviceExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    StringArrayAddElement(&deviceExtensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    StringArrayAddElement(&deviceExtensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    {
        // device creation
        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR deviceaddr = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
            .bufferDeviceAddress = VK_TRUE,
        };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStruc = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .accelerationStructure = VK_TRUE,
            .pNext = &deviceaddr,
        };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytrace = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .rayTracingPipeline = VK_TRUE,
            .pNext = &accelStruc,
        };
        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynrendering = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .dynamicRendering = VK_TRUE,
            .pNext = &raytrace,
        };
        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .features = {0},
            .pNext = &dynrendering,
        };
        
        result = createDevice(
            state->instance,
            state->surface,
            &features,
            deviceLayers,
            deviceExtensions,
            &state->device
        );
    }
    StringArrayDestroy(&deviceExtensions);
    StringArrayDestroy(&deviceLayers);

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
    const RtVertex vertices[] = {
        {-0.8f, -0.8f, 0.0f},
        {0.8f, -0.8f, 0.0f},
        {0.8f, 0.8f, 0.0f},
        {-0.8f, 0.8f, 0.0f},
    };
    const uint32_t indices[] = {
        0, 2, 1,
        0, 3, 2,
    };

    state->mesh = CreateRtMesh(
        state,
        vertices, sizeof(vertices) / sizeof(RtVertex),
        indices, sizeof(indices) / sizeof(uint32_t)
    );

    state->blas = CreateAccelerationStructure(
        state,
        state->mesh.vertexBuffer.buffer,
        sizeof(vertices) / sizeof(RtVertex),
        state->mesh.indexBuffer.buffer,
        sizeof(indices) / sizeof(uint32_t)
    );

    CreateRayTracingPipeline(state);

    return state;
}

void destroyVulkanState(VulkanState *vulkanState) {
    assert(waitIdle(&vulkanState->device) == VK_SUCCESS);

    destroyAccelerationStructureKHR(&vulkanState->device, vulkanState->blas.structure);
    destroyDeallocateBuffer(vulkanState->allocator, &vulkanState->blas.buffer);
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

    destroyPipeline(&vulkanState->device, vulkanState->rayTracingPipeline);
    destroyPipeline(&vulkanState->device, vulkanState->graphicsPipeline);
    destroyPipelineLayout(&vulkanState->device, vulkanState->rayTracingLayout);
    destroyPipelineLayout(&vulkanState->device, vulkanState->layout);
    destroyDescriptorSetLayout(&vulkanState->device, vulkanState->rayTracingSetLayout);

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

    transitionImageLayout(cmdBuffer, vulkanState->swapchain.images[imageIndex],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );

    VkClearValue clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderingAttachmentInfoKHR attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .clearValue = clearValue,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .imageView = vulkanState->swapchain.imageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .resolveMode = VK_RESOLVE_MODE_NONE,
    };

    VkRenderingInfoKHR renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pColorAttachments = &attachment,
        .colorAttachmentCount = 1,
        .pDepthAttachment = VK_NULL_HANDLE,
        .pStencilAttachment = VK_NULL_HANDLE,
        .viewMask = 0,
        .layerCount = 1,
        .renderArea = {
            {0, 0},
            vulkanState->swapchain.extent,
        },
    };
    cmdBeginRenderingKHR(&vulkanState->device, cmdBuffer, &renderInfo);
    {
        // cmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanState->graphicsPipeline);

        // VkViewport viewport = {
        //     .x = 0.0f, .y = 0.0f,
        //     .width = (float)vulkanState->swapchain.extent.width,
        //     .height = (float)vulkanState->swapchain.extent.height,
        //     .minDepth = 0.0f,
        //     .maxDepth = 1.0f,
        // };
        // cmdSetViewport(cmdBuffer, viewport);

        // VkRect2D scissor = {
        //     .offset = {0, 0},
        //     .extent = vulkanState->swapchain.extent,
        // };
        // cmdSetScissor(cmdBuffer, scissor);

        // VkBuffer vertexBuffers[] = {vulkanState->mesh.vertexBuffer.buffer};
        // VkDeviceSize offsets[] = {0};
        // cmdBindVertexBuffers(
        //     cmdBuffer,
        //     (UInt32Range){0, 1},
        //     vertexBuffers,
        //     offsets
        // );
        // cmdBindIndexBuffer(
        //     cmdBuffer,
        //     vulkanState->mesh.indexBuffer.buffer,
        //     0,
        //     VK_INDEX_TYPE_UINT32
        // );
        // cmdDrawIndexed(
        //     cmdBuffer,
        //     (UInt32Range){0, vulkanState->mesh.indexCount},
        //     (UInt32Range){0, 1},
        //     0
        // );
    }
    cmdEndRenderingKHR(&vulkanState->device, cmdBuffer);

    transitionImageLayout(cmdBuffer, vulkanState->swapchain.images[imageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_NONE,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    );

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
}

void framebufferResized(VulkanState *vulkanState) {
    vulkanState->framebufferResized = VK_TRUE;
}

#include <main.frag.h>
#include <main.vert.h>

void CreateGraphicsPipeline(VulkanState *state) {
    VkResult result;

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

    VkPipelineRenderingCreateInfoKHR pipelineRendering = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &state->swapchain.format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .viewMask = 0,
    };

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
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
        .pNext = &pipelineRendering
    };
    result = createGraphicsPipeline(&state->device, &pipelineInfo, &state->graphicsPipeline);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline: %s.\n", string_VkResult(result));
        exit(1);
    }

    destroyShaderModule(&state->device, fragment);
    destroyShaderModule(&state->device, vertex);
}

#include <ray.rgen.h>
#include <ray.rchit.h>
#include <ray.rmiss.h>

void CreateRayTracingPipeline(VulkanState *state) {
    VkResult result;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };
    VkPhysicalDeviceProperties2 deviceProperties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rayTracingProperties,
    };
    getPhysicalDeviceProperties2(state->instance, state->device.physicalDevice, &deviceProperties);

    VkShaderModule raygen, miss, closestHit;
    result = createShaderModule(
        &state->device,
        (const uint32_t*)ray_rgen_h,
        sizeof(ray_rgen_h),
        &raygen
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module: %s.\n", string_VkResult(result));
        exit(1);
    }

    result = createShaderModule(
        &state->device,
        (const uint32_t*)ray_rmiss_h,
        sizeof(ray_rmiss_h),
        &miss
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module: %s.\n", string_VkResult(result));
        exit(1);
    }

    result = createShaderModule(
        &state->device,
        (const uint32_t*)ray_rchit_h,
        sizeof(ray_rchit_h),
        &closestHit
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkPipelineShaderStageCreateInfo raygenStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .module = raygen,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo missStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module = miss,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo closestHitStage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module = closestHit,
        .pName = "main",
    };

    PipelineStageArray array = PipelineStageArrayNew(10);
    PipelineStageArrayAddElement(&array, raygenStage);
    PipelineStageArrayAddElement(&array, missStage);
    PipelineStageArrayAddElement(&array, closestHitStage);

    VkRayTracingShaderGroupCreateInfoKHR raygenGroup = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .generalShader = 0,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR missGroup = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .generalShader = 1,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR closestHitGroup = {
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .generalShader = VK_SHADER_UNUSED_KHR,
        .anyHitShader = VK_SHADER_UNUSED_KHR,
        .closestHitShader = 2,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
    };

    VkRayTracingShaderGroupCreateInfoKHR groups[] = {
        raygenGroup, missGroup, closestHitGroup,
    };

    // Setup pipeline
    VkDescriptorSetLayoutBinding bindings[] = {
        (VkDescriptorSetLayoutBinding){
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
        }
    };
    VkDescriptorSetLayoutCreateInfo rayTracingSetLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pBindings = bindings,
        .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
    };
    result = createDescriptorSetLayout(&state->device, &rayTracingSetLayoutInfo, &state->rayTracingSetLayout);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkDescriptorSetLayout setLayouts[] = {
        state->rayTracingSetLayout,
    };
    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = setLayouts,
        .setLayoutCount = sizeof(setLayouts) / sizeof(VkDescriptorSetLayout),
        .pPushConstantRanges = NULL,
        .pushConstantRangeCount = 0,
    };
    result = createPipelineLayout(&state->device, &layoutInfo, &state->rayTracingLayout);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamicStates,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
    };

    result = createRayTracingPipelineKHR(
        &state->device,
        state->rayTracingLayout,
        &dynamicStateInfo,
        groups,
        sizeof(groups) / sizeof(VkRayTracingShaderGroupCreateInfoKHR),
        array,
        &state->rayTracingPipeline
    );
    PipelineStageArrayDestroy(&array);
    destroyShaderModule(&state->device, raygen);
    destroyShaderModule(&state->device, miss);
    destroyShaderModule(&state->device, closestHit);

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create ray tracing pipeline: %s.\n", string_VkResult(result));
        exit(1);
    }

    size_t handlesBufferSize = rayTracingProperties.shaderGroupHandleSize *
        (sizeof(groups) / sizeof(VkRayTracingShaderGroupCreateInfoKHR));
    void *handles = malloc(handlesBufferSize);
    result = getRayTracingShaderGroupHandlesKHR(
        &state->device, state->rayTracingPipeline,
        0, 3,
        handlesBufferSize, handles
    );

    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get shader group handles: %s.\n", string_VkResult(result));
        exit(1);
    }

    free(handles);
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

Mesh CreateRtMesh(
    VulkanState *state,
    const RtVertex *vertices,
    size_t vertexCount,
    const uint32_t *indices,
    size_t indexCount
) {
    Mesh mesh;

    mesh.vertexBuffer = CreateBufferGQueue(
        state,
        sizeof(Vertex) * vertexCount,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );
    mesh.indexBuffer = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );

    Buffer vertexStaging = CreateBufferGQueue(
        state,
        sizeof(Vertex) * vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        0
    );
    Buffer indexStaging = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        0
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
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0
    );
    mesh.indexBuffer = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0
    );

    Buffer vertexStaging = CreateBufferGQueue(
        state,
        sizeof(Vertex) * vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        0
    );
    Buffer indexStaging = CreateBufferGQueue(
        state,
        sizeof(uint32_t) * indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        0
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

Buffer CreateBufferGQueue(
    VulkanState *state,
    VkDeviceSize bufferSize,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    VkMemoryAllocateFlags allocateFlags
) {
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pQueueFamilyIndices = &state->device.queueFamilies.graphics,
        .queueFamilyIndexCount = 1,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .size = bufferSize,
    };
    Buffer buffer;
    assert(createAllocateBuffer(
        state->allocator,
        &bufferInfo,
        memoryFlags,
        allocateFlags,
        &buffer
    ) == VK_SUCCESS);

    return buffer;
}

AccelerationStructure CreateAccelerationStructure(
    VulkanState *state,
    VkBuffer vertexBuffer,
    uint32_t vertexCount,
    VkBuffer indexBuffer,
    uint32_t indexCount
) {
    VkResult result;

    // Request build size
    VkDeviceAddress vertexAddress = getBufferAddressKHR(&state->device, vertexBuffer);
    VkDeviceAddress indexAddress = getBufferAddressKHR(&state->device, indexBuffer);
    VkAccelerationStructureGeometryKHR geometry = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry.triangles = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .transformData.deviceAddress = 0,
            .vertexData.deviceAddress = vertexAddress,
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexStride = sizeof(Vector3f),
            .maxVertex = vertexCount - 1,
            .indexData.deviceAddress = indexAddress,
            .indexType = VK_INDEX_TYPE_UINT32,
        },
    };

    VkAccelerationStructureBuildGeometryInfoKHR geometryBuildInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .pGeometries = &geometry,
        .geometryCount = 1,

        // To be filled after creating blas
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .scratchData.deviceAddress = 0,
    };

    VkAccelerationStructureBuildSizesInfoKHR geometryBuildSizes = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    UInt32Array primitiveCounts = UInt32ArrayNew(1);
    UInt32ArrayAddElement(&primitiveCounts, indexCount / 3);

    getAccelerationStructureBuildSizesKHR(
        &state->device,
        primitiveCounts,
        &geometryBuildInfo,
        &geometryBuildSizes
    );

    UInt32ArrayDestroy(&primitiveCounts);

    // Create acceleration structure and buffers
    Buffer accelerationStructureBuffer = CreateBufferGQueue(
        state,
        geometryBuildSizes.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0
    );
    Buffer scratchBuffer = CreateBufferGQueue(
        state,
        geometryBuildSizes.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    );

    VkAccelerationStructureCreateInfoKHR structureInfo = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .size = geometryBuildSizes.accelerationStructureSize,
        .buffer = accelerationStructureBuffer.buffer,
    };
    VkAccelerationStructureKHR blas;
    result = createAccelerationStructureKHR(
        &state->device,
        &structureInfo,
        &blas
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create acceleration structure: %s.\n", string_VkResult(result));
        exit(1);
    }

    // Assign buffers and blas
    VkDeviceAddress scratchAddress = getBufferAddressKHR(&state->device, scratchBuffer.buffer);
    geometryBuildInfo.dstAccelerationStructure = blas;
    geometryBuildInfo.scratchData.deviceAddress = scratchAddress;

    // Create command buffer for build
    VkCommandBuffer cmdBuffer;
    result = allocateCommandBuffer(&state->device, state->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &cmdBuffer);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffer: %s.\n", string_VkResult(result));
        exit(1);
    }

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .firstVertex = 0,
        .primitiveOffset = 0,
        .transformOffset = 0,
        .primitiveCount = indexCount / 3,
    };

    ASGeometryBuildInfoArray buildInfoArray = ASGeometryBuildInfoArrayNew(10);
    ASBuildRangeInfoArray buildRangeArray = ASBuildRangeInfoArrayNew(10);

    ASGeometryBuildInfoArrayAddElement(&buildInfoArray, geometryBuildInfo);
    ASBuildRangeInfoArrayAddElement(&buildRangeArray, buildRangeInfo);

    assert(beginOneTimeCommandBuffer(cmdBuffer) == VK_SUCCESS);
    {
        cmdBuildAccelerationStructuresKHR(
            &state->device,
            cmdBuffer,
            buildInfoArray,
            buildRangeArray
        );
    }
    assert(endCommandBuffer(cmdBuffer) == VK_SUCCESS);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = &cmdBuffer,
        .commandBufferCount = 1,
    };
    assert(queueSubmit(state->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) == VK_SUCCESS);
    assert(queueWaitIdle(state->graphicsQueue) == VK_SUCCESS);

    ASGeometryBuildInfoArrayDestroy(&buildInfoArray);
    ASBuildRangeInfoArrayDestroy(&buildRangeArray);

    freeCommandBuffers(&state->device, state->commandPool, &cmdBuffer, 1);

    destroyDeallocateBuffer(state->allocator, &scratchBuffer);

    // We finished, we can return
    return (AccelerationStructure){
        .structure = blas,
        .buffer = accelerationStructureBuffer,
    };
}
