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
void onKeyEvent(GLFWwindow *w, int key, int scancode, int action, int mode);

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

void onKeyEvent(GLFWwindow *w, int key, int scancode, int action, int mode) {
    (void)scancode;
    (void)mode;
    
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    }
}

int main(void) {
    window = createWindow();
#ifndef RELEASE
    state = initVulkanState(&window, VK_TRUE);
#else
    state = initVulkanState(&window, VK_FALSE);
#endif

    glfwSetWindowSizeCallback(window.window, onResize);
    glfwSetKeyCallback(window.window, onKeyEvent);

    while(!windowShouldClose(&window)) {
        pollEvents();

        // Vulkan drawing
        drawFrame();
    }

    destroyVulkanState(state);
    destroyWindow(&window);

    return 0;
}


