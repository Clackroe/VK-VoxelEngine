#include "GLFW/glfw3.h"
#include "vulkan.hpp"
#include "window.hpp"
#include <Arena.h>
#include <cstdio>
#include <cstring>
#include <vulkan/vulkan_core.h>

#define RES_FACTOR 150

#define SCREEN_WIDTH 16 * RES_FACTOR
#define SCREEN_HEIGHT 9 * RES_FACTOR

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanContext* ctx = (VulkanContext*)glfwGetWindowUserPointer(window);
    ctx->frame_buffer_resized = true;
}

void draw(Arena* arr, VulkanContext* ctx, Window* window)
{
    vkWaitForFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, ctx->image_available_semaphores[ctx->current_frame], VK_NULL_HANDLE, &imageIndex);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || ctx->frame_buffer_resized) {
        ctx->frame_buffer_resized = false;
        recreate_swapchain(arr, ctx, window);
        return;
    } else if (res != VK_SUCCESS) {
        printf("Failed to accquure next swapchain image");
    }

    vkResetFences(ctx->device, 1, &ctx->in_flight_fences[ctx->current_frame]);

    vkResetCommandBuffer(ctx->cmd_buffers[ctx->current_frame], 0);
    record_command_buffer(ctx, ctx->cmd_buffers[ctx->current_frame], imageIndex);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { ctx->image_available_semaphores[ctx->current_frame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx->cmd_buffers[ctx->current_frame];

    VkSemaphore signalSemaphores[] = { ctx->render_finished_semaphores[ctx->current_frame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_CHECK_RESULT(vkQueueSubmit(ctx->graphics_queue, 1, &submitInfo, ctx->in_flight_fences[ctx->current_frame]));

    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { ctx->swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(ctx->present_queue, &presentInfo);

    ctx->current_frame = (ctx->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main()
{

    Arena* GameArena = create_arena(10 MB);

    Window* window = Window::Create(GameArena, SCREEN_WIDTH, SCREEN_HEIGHT);

    VulkanContext* ctx = VulkanContext::Create(GameArena, window);

    glfwSetKeyCallback(window->window, key_callback);

    while (!glfwWindowShouldClose(window->window)) {

        window->update();
        draw(GameArena, ctx, window);
    }

    vkDeviceWaitIdle(ctx->device);

    cleanup_vulkan(ctx, window);

    print_arena(GameArena);
    arena_free(GameArena);
}
