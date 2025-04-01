#include "window.h"
#include <GLFW/glfw3.h>

Window createWindow(void) {
    Window window;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window.window = glfwCreateWindow(1280, 720, "My app", NULL, NULL);

    return window;
}

int windowShouldClose(Window *window) {
    return glfwWindowShouldClose(window->window);
}

void pollEvents(void) {
    glfwPollEvents();
}

void destroyWindow(Window *window) {
    glfwDestroyWindow(window->window);
    glfwTerminate();
}

void getFramebufferSize(Window *window, int *width, int *height) {
    glfwGetFramebufferSize(window->window, width, height);
}
