#include "GLFW/glfw3.h"
#include "vulkan.hpp"
#include "window.hpp"
#include <Arena.h>
#include <cstdio>
#include <cstring>

#define RES_FACTOR 8

#define SCREEN_WIDTH 16 * RES_FACTOR
#define SCREEN_HEIGHT 9 * RES_FACTOR

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main()
{
    Arena* GameArena = create_arena(25 MB);

    Window* window = Window::Create(GameArena, SCREEN_WIDTH, SCREEN_HEIGHT);

    VulkanContext* ctx = VulkanContext::Create(GameArena, window);

    glfwSetKeyCallback(window->window, key_callback);

    while (!glfwWindowShouldClose(window->window)) {

        window->update();
    }

    cleanup_vulkan(ctx, window);

    print_arena(GameArena);
    arena_free(GameArena);
}
