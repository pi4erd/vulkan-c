#include "app.h"
#include "device_api.h"
#include "engine.h"
#include "swapchain.h"

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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
    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSema, renderFinishedSema;
    VkFence inFlightFence;

    VkBool32 portability;
} VulkanState;

void CreateGraphicsPipeline(VulkanState *state);

VulkanState *initVulkanState(Window *window, VkBool32 debugging) {
    StringArray extensions = createStringArray(1000);
    StringArray layers = createStringArray(1000);

    VulkanState *state = calloc(1, sizeof(VulkanState));

    uint32_t glfwRequiredExtensionCount;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    
    appendStringArray(&extensions, glfwRequiredExtensions, glfwRequiredExtensionCount);
    addStringToArray(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    if(debugging) {
        addStringToArray(&layers, "VK_LAYER_KHRONOS_validation");
        addStringToArray(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
        addStringToArray(&deviceExtensions, "VK_KHR_portability_subset");
    }
    addStringToArray(&deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if(debugging) {
        addStringToArray(&deviceLayers, "VK_LAYER_KHRONOS_validation");
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

    result = allocateCommandBuffer(
        &state->device,
        state->commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        &state->commandBuffer
    );
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffer: %s.\n", string_VkResult(result));
        exit(1);
    }

    assert(createSemaphore(&state->device, &state->imageAvailableSema) == VK_SUCCESS);
    assert(createSemaphore(&state->device, &state->renderFinishedSema) == VK_SUCCESS);
    assert(createFence(&state->device, VK_TRUE, &state->inFlightFence) == VK_SUCCESS);

    return state;
}

void destroyVulkanState(VulkanState *vulkanState) {
    assert(waitIdle(&vulkanState->device) == VK_SUCCESS);

    destroySemaphore(&vulkanState->device, vulkanState->imageAvailableSema);
    destroySemaphore(&vulkanState->device, vulkanState->renderFinishedSema);
    destroyFence(&vulkanState->device, vulkanState->inFlightFence);

    destroyCommandPool(&vulkanState->device, vulkanState->commandPool);

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
    assert(resetCommandBuffer(vulkanState->commandBuffer) == VK_SUCCESS);
    assert(beginSimpleCommandBuffer(vulkanState->commandBuffer) == VK_SUCCESS);

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
    cmdBeginRenderPass(vulkanState->commandBuffer, &beginInfo);
    {
        cmdBindPipeline(vulkanState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanState->graphicsPipeline);

        VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = (float)vulkanState->swapchain.extent.width,
            .height = (float)vulkanState->swapchain.extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        cmdSetViewport(vulkanState->commandBuffer, viewport);

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = vulkanState->swapchain.extent,
        };
        cmdSetScissor(vulkanState->commandBuffer, scissor);
        cmdDraw(vulkanState->commandBuffer, (UInt32Range){0, 3}, (UInt32Range){0, 1});
    }
    cmdEndRenderPass(vulkanState->commandBuffer);

    assert(endCommandBuffer(vulkanState->commandBuffer) == VK_SUCCESS);
}

void syncStartFrame(VulkanState *vulkanState) {
    assert(waitForFence(&vulkanState->device, vulkanState->inFlightFence, UINT64_MAX) == VK_SUCCESS);
    assert(resetFence(&vulkanState->device, vulkanState->inFlightFence) == VK_SUCCESS);
}

VkResult getImage(VulkanState *vulkanState, uint32_t *image) {
    VkResult result = acquireNextImage(
        &vulkanState->device,
        &vulkanState->swapchain,
        UINT64_MAX,
        vulkanState->imageAvailableSema,
        VK_NULL_HANDLE,
        image
    );

    return result;
}

void renderAndPresent(VulkanState *vulkanState, uint32_t imageIndex) {
    VkSemaphore waitSemaphores[] = {vulkanState->imageAvailableSema};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {vulkanState->renderFinishedSema};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(VkSemaphore),
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .pCommandBuffers = &vulkanState->commandBuffer,
        .commandBufferCount = 1,
        .pSignalSemaphores = signalSemaphores,
        .signalSemaphoreCount = 1,
    };

    VkResult result;
    result = queueSubmit(vulkanState->graphicsQueue, 1, &submitInfo, vulkanState->inFlightFence);
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
    if(result != VK_SUCCESS) {
        fprintf(stderr, "Failed to queue present: %s.\n", string_VkResult(result));
        return;
    }
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

    VkPipelineVertexInputStateCreateInfo inputStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
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
