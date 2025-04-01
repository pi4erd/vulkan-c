#include "app.h"
#include "window.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <assert.h>
#include <vulkan/vk_enum_string_helper.h>

static Window window;
static VulkanState *state;

void drawFrame(void);
void onResize(GLFWwindow *w, int width, int height);

void drawFrame() {
    uint32_t imageIndex;
    if(getImage(state, &window, &imageIndex) == VK_FALSE) {
        return;
    }

    recordCommandBuffer(state, imageIndex);

    renderAndPresent(state, &window, imageIndex);
}

void onResize(GLFWwindow *w, int width, int height) {
    (void)w;
    if(width == 0 && height == 0) { return; }

    framebufferResized(state);
    // recreateSwapChain(state, &window);
}

int main(void) {
    window = createWindow();
    state = initVulkanState(&window, VK_TRUE);

    glfwSetWindowSizeCallback(window.window, onResize);

    while(!windowShouldClose(&window)) {
        pollEvents();

        // Vulkan drawing
        drawFrame();
    }

    destroyVulkanState(state);
    destroyWindow(&window);

    return 0;
}


