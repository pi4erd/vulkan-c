#ifndef WINDOW_H_
#define WINDOW_H_

#include <GLFW/glfw3.h>

typedef struct {
    GLFWwindow *window;
} Window;

Window createWindow(void);
int windowShouldClose(Window *window);
void pollEvents(void);
void destroyWindow(Window *window);

#endif
