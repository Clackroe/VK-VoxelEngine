#include <cstring>

#include "GLFW/glfw3.h"
#include <cstdio>
#include <window.hpp>

Window* Window::Create(Arena* arr, uint32_t width, uint32_t height)
{

    Window* window = (Window*)arena_allocate(arr, sizeof(Window));
    memset(window, 0, sizeof(*window));

    window->width = width;
    window->height = height;

    if (!glfwInit()) {
        printf("Failed to init GLFW");
        glfwTerminate();
        return nullptr;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->window = glfwCreateWindow(width, height, "RaycastingEngine", NULL, NULL);
    if (!window->window) {
        glfwTerminate();
        printf("Failed to init Window");
        return nullptr;
    }

    return window;
}

void Window::update()
{
    glfwPollEvents();
}
void Window::destroy()
{
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Window::displayBytes(unsigned char* bytes, int width, int height)
{
    // glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
}
