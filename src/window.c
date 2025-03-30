#include "window.h"

Window createWindow(void) {
    Window window;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
