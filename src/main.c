#include "app.h"

#include <vulkan/vulkan_core.h>
#include <assert.h>

void drawFrame(VulkanState *state);

void drawFrame(VulkanState *state) {
    syncStartFrame(state);

    uint32_t imageIndex;
    VkResult getImageResult = getImage(state, &imageIndex);
    assert(getImageResult == VK_SUCCESS); // TODO: Handle if we fail to get image
    recordCommandBuffer(state, imageIndex);

    renderAndPresent(state, imageIndex);
}

int main(void) {
    Window window = createWindow();
    VulkanState *state = initVulkanState(&window, VK_TRUE);

    while(!windowShouldClose(&window)) {
        pollEvents();

        // Vulkan drawing
        drawFrame(state);
    }

    destroyVulkanState(state);
    destroyWindow(&window);

    return 0;
}


