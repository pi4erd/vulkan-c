#include "app.h"

int main(void) {
    Window window = createWindow();
    VulkanState *state = initVulkanState(&window, VK_TRUE);

    while(!windowShouldClose(&window)) {
        pollEvents();
    }

    destroyVulkanState(state);
    destroyWindow(&window);

    return 0;
}


