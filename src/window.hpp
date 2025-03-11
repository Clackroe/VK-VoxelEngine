#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "GLFW/glfw3.h"
#include <Arena.h>
#include <cstdint>

struct Window {
    GLFWwindow* window;
    int width = 0;
    int height = 0;

    static Window* Create(Arena* arr, uint32_t width, uint32_t height);

    void update();
    void destroy();
    void displayBytes(unsigned char* bytes, int width, int height);
};

#endif // WINDOW_HPP
